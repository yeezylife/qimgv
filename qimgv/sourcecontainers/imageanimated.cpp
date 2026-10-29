#include "imageanimated.h"

#include <QFile>
#include <QGlobalStatic>
#include <QMutexLocker>
#include <QRunnable>
#include <QThreadPool>
#include <QWaitCondition>
#include <atomic>
#include <cstddef>
#include <deque>
#include <utility>

// 帧缓存 + 解码状态的共享结构。
// ⭐ 线程约定：QImageReader 由后台解码任务在自己的线程创建/销毁（不跨线程复用），
// GUI 线程只在锁内读/取已发布的帧；对象析构后任务通过 shared_ptr 继续持有本结构，
// 由 shutdown 标志让其自行退出，不会出现悬垂指针。
struct ImageAnimated::DecodeContext {
    struct CachedFrame {
        int index = 0;
        std::shared_ptr<const QImage> image;
        int delay = 10;   // 该帧的显示时长，随帧一起回收（元数据同样有界）
    };

    QMutex mutex;
    QWaitCondition wake;                  // 播放头前进/停止时唤醒解码任务
    std::deque<CachedFrame> frames;       // 连续窗口 [front().index, nextIndex)
    int currentDelay = 10;                // 当前显示帧的延迟（帧被回收后仍需保留）
    QString path;                         // 解码任务在自己的线程里创建 reader
    int allocationLimitMB = 0;
    int nextIndex = 0;                    // reader->read() 下一次产出的帧号
    qsizetype cachedBytes = 0;
    bool exhausted = false;               // reader 已读到末尾（受 mutex 保护）

    std::atomic<bool> wantMore{true};   // 视图是否仍需要后续帧（false = 暂停）
    std::atomic<bool> shutdown{false};  // 解码任务是否应立即退出（stopDecoding/析构）
    std::atomic<bool> running{false};   // 是否有解码任务在跑（暂停时任务仍挂起，为 true）
    std::atomic<int> playhead{0};       // 当前播放帧号
    std::atomic<int> requestedFrame{-1}; // 随机跳转请求（-1 = 无）
    std::atomic<int> reportedFrameCount{0}; // imageCount() 的扫描值，EOF 时用于修正
    // EOF 后修正的总帧数：-1 = 尚未修正，>0 = 已确认的真实帧数。
    // 0 不是有效修正值（读取失败/暂停/reader 暂不可用都不代表零帧动画）。
    std::atomic<int> correctedFrameCount{-1};

    // 调用方必须持有 mutex
    bool frameAtLocked(int index, std::shared_ptr<const QImage> &out,
                       int *delay = nullptr) const {
        if (index < 0 || frames.empty())
            return false;
        const int first = frames.front().index;
        if (index < first)
            return false;
        const std::size_t offset = static_cast<std::size_t>(index - first);
        if (offset >= frames.size())
            return false;
        out = frames[offset].image;
        if (delay)
            *delay = frames[offset].delay;
        return true;
    }
};

namespace {
// 解码前瞻窗口与环形缓冲上限：内存有界，播放头离开窗口后旧帧才可回收。
// 达到帧数/字节上限时解码任务暂停等待，播放头前进后自然继续。
constexpr int kLookaheadFrames = 24;
constexpr int kHistoryFrames = 8;   // 播放头之后保留少量已显示帧，支持逐帧后退
constexpr int kMaxCachedFrames = 64;
constexpr qsizetype kMaxCachedBytes = static_cast<qsizetype>(128) * 1024 * 1024;

// 全局单线程池：同一时刻只显示一个动画，串行预解码即可。
// Q_GLOBAL_STATIC 在 QCoreApplication 析构阶段清理（qAddPostRoutine），
// 不会像函数内静态对象那样活到 qApp 之后。
Q_GLOBAL_STATIC(QThreadPool, frameDecodePoolInstance)

QThreadPool *frameDecodePool() {
    static QThreadPool *pool = []() {
        QThreadPool *p = frameDecodePoolInstance();
        p->setMaxThreadCount(1);
        return p;
    }();
    return pool;
}

qsizetype frameBytes(const ImageAnimated::DecodeContext::CachedFrame &frame) {
    return frame.image ? frame.image->sizeInBytes() : 0;
}

using FrameDeque = std::deque<ImageAnimated::DecodeContext::CachedFrame>;

// 前缀定位/单帧读取期间的任务有效性：停止、暂停或有不同的新定位请求时，
// 当前任务的结果都不应再写入窗口。read() 之前和之后都必须复查。
// activeTarget 是本次正在补齐的帧号：GUI 的重试定时器会反复提交同一个目标，
// 把"目标相同"的请求当成新请求会让长动画的前缀定位反复重启（永远追不上）。
bool decodeCancelled(const ImageAnimated::DecodeContext &ctx, int activeTarget) {
    if (ctx.shutdown.load(std::memory_order_acquire)
        || !ctx.wantMore.load(std::memory_order_acquire))
        return true;
    const int requested = ctx.requestedFrame.load(std::memory_order_acquire);
    return requested >= 0 && requested != activeTarget;
}

enum class PrefixOpenStatus {
    Ready,
    Cancelled,   // 任务已过期，必须丢弃 reader 与本次定位结果
    EndOfFile    // 正常读到文件末尾，可按实际帧数修正
};

struct PrefixOpenResult {
    std::unique_ptr<QImageReader> reader;
    PrefixOpenStatus status = PrefixOpenStatus::Ready;
    int decoded = 0;   // 从文件头成功读出的帧数（EndOfFile 时小于请求的前缀长度）
};

// 在调用线程内新建 reader，并顺序解码丢弃前 frameIndex 帧。
// reader 必须由实际执行 read() 的线程创建/销毁（内部持有 QFile），
// 因此 DecodeContext 不保存 reader；任务重启时按已解码帧号重新定位。
// 注：Qt 的动画 handler（GIF/WebP 等）不实现 jumpToImage（默认返回 false），
// 且 WebP 的探测会 readAll() 整个文件，因此不做"先试随机寻址"的探测，
// 顺序丢弃是唯一可行的定位方式。
//
// 长动画的前缀定位可能持续数秒：每帧 read() 之间都检查取消状态，
// 不能让它占住单线程解码池去追一个已经过期的跳转目标。
PrefixOpenResult openReaderAt(const QString &path, int frameIndex,
                              int allocationLimitMB, int activeTarget,
                              const ImageAnimated::DecodeContext &ctx) {
    PrefixOpenResult result;
    result.reader = std::make_unique<QImageReader>(path);
    result.reader->setAllocationLimit(allocationLimitMB);

    for (int i = 0; i < frameIndex; ++i) {
        if (decodeCancelled(ctx, activeTarget)) {
            result.reader.reset();
            result.status = PrefixOpenStatus::Cancelled;
            return result;
        }
        if (result.reader->read().isNull()) {
            // 这是真实 EOF，不是取消：调用方可据此修正帧数
            result.status = PrefixOpenStatus::EndOfFile;
            result.decoded = i;
            return result;
        }
        result.decoded = i + 1;
    }

    // 前缀读完到返回之间仍可能有停止/暂停/不同目标的新跳转到达
    if (decodeCancelled(ctx, activeTarget)) {
        result.reader.reset();
        result.status = PrefixOpenStatus::Cancelled;
    }
    return result;
}

// 清空环形窗口（调用方持锁）：待释放帧移入 doomed，由调用方在锁外析构，
// 避免 GUI 线程在锁上等待大批 QImage 的释放
void detachAllFramesLocked(ImageAnimated::DecodeContext &ctx, FrameDeque &doomed) {
    for (auto &frame : ctx.frames)
        doomed.push_back(std::move(frame));
    ctx.frames.clear();
    ctx.cachedBytes = 0;
}

// 停止后回收整个帧窗口，并让 reader 状态与空窗口保持一致：
// 下次启动会从 0 重建 reader。correctedFrameCount 保留（真实帧数仍有效），
// 当前显示帧由 ImageAnimated::cachedFrame 持有。
void resetWindowLocked(ImageAnimated::DecodeContext &ctx, FrameDeque &doomed) {
    detachAllFramesLocked(ctx, doomed);
    ctx.nextIndex = 0;
    ctx.exhausted = false;
}

// 文件在两次打开之间变短时，窗口尾部可能留下 index >= endIndex 的旧帧。
// 保持 [front().index, nextIndex) 的连续窗口不变式，否则后续 read() 会以
// nextIndex 为序号追加，和残留帧产生重复/乱序。
void trimWindowEndLocked(ImageAnimated::DecodeContext &ctx, int endIndex,
                         FrameDeque &doomed) {
    while (!ctx.frames.empty() && ctx.frames.back().index >= endIndex) {
        ctx.cachedBytes -= frameBytes(ctx.frames.back());
        doomed.push_back(std::move(ctx.frames.back()));
        ctx.frames.pop_back();
    }
}

void trimLocked(ImageAnimated::DecodeContext &ctx, int playhead, FrameDeque &doomed) {
    while (ctx.frames.size() > 1) {
        const bool overFrames = ctx.frames.size() > static_cast<std::size_t>(kMaxCachedFrames);
        const bool overBytes = ctx.cachedBytes > kMaxCachedBytes;
        const auto &front = ctx.frames.front();
        // 播放头离开历史窗口后即可回收（滑动窗口），不必等超出硬上限：
        // 否则缓冲填满后解码任务会停在上限处，前瞻消失
        const bool pastHistory = front.index + kHistoryFrames < playhead;
        if (!overFrames && !overBytes && !pastHistory)
            break;
        // 播放头之后的帧尚未显示，不能为限额丢弃（否则会解码→驱逐→再解码死循环）；
        // 达到上限时解码任务暂停，等播放头前进后再回收
        if (front.index >= playhead)
            break;
        ctx.cachedBytes -= frameBytes(front);
        doomed.push_back(std::move(ctx.frames.front()));
        ctx.frames.pop_front();
    }
}

// 只有本次实际解出的帧数 > 0 才能更新修正值。读取失败时 decoded 为 0，
// 不能把“reader 暂时不可用”写成“已确认 0 帧”；已有修正值（-1 或正数）保持不变。
void storeCorrectedFrameCount(ImageAnimated::DecodeContext &ctx, int decoded) {
    if (decoded <= 0)
        return;
    const int reported = ctx.reportedFrameCount.load(std::memory_order_relaxed);
    ctx.correctedFrameCount.store(
        reported > 0 ? qMin(decoded, reported) : decoded,
        std::memory_order_release);
}

// 后台预解码任务：按播放头维护有界窗口，只解码"当前帧 + 前瞻"所需的数据。
// 注意：任务不持有 ImageAnimated，只能访问 DecodeContext，
// 因此对象被销毁后任务仍可安全跑完当前帧再退出。
class FrameDecoderTask : public QRunnable {
public:
    explicit FrameDecoderTask(std::shared_ptr<ImageAnimated::DecodeContext> ctx)
        : m_ctx(std::move(ctx)) {}

    void run() override {
        ImageAnimated::DecodeContext &ctx = *m_ctx;
        FrameDeque doomed;   // 待释放帧：在锁外析构，避免大 QImage 释放阻塞 GUI
        // reader 只在当前 worker 线程创建和销毁，线程亲和性始终正确。
        // 延迟到真正需要解码时再创建：任务排队期间若已被 stopDecoding() 取消，
        // 这里会先看到 shutdown 直接退出，不会白做一次昂贵的顺序前缀定位。
        std::unique_ptr<QImageReader> reader;

        for (;;) {
            doomed.clear();   // 上一轮取出的待释放帧在此（锁外）析构

            int target = -1;        // 本次 read() 之后的顺序补齐目的地
            bool needsRewind = false;
            int decodeFrom = 0;     // 新建 reader 时顺序丢弃的前缀帧数
            {
                QMutexLocker locker(&ctx.mutex);

                for (;;) {
                    if (ctx.shutdown.load(std::memory_order_acquire))
                        break;
                    if (!ctx.wantMore.load(std::memory_order_acquire)) {
                        // 暂停：保留 reader 与已解码窗口，等恢复后原地继续。
                        // GIF/WebP 无法随机寻址，退出任务再重建的代价是重解码整个前缀
                        ctx.wake.wait(&ctx.mutex, 100);
                        continue;
                    }

                    // 随机跳转：目标已在窗口内直接继续；在窗口之前（含已回收）
                    // 必须重建 reader 从 0 解码，之后顺序补齐即可
                    const int requested = ctx.requestedFrame.exchange(-1, std::memory_order_acq_rel);
                    if (requested >= 0) {
                        std::shared_ptr<const QImage> have;
                        if (ctx.frameAtLocked(requested, have))
                            continue;
                        target = requested;
                        needsRewind = requested < ctx.nextIndex;
                        break;
                    }

                    const int head = ctx.playhead.load(std::memory_order_relaxed);
                    std::shared_ptr<const QImage> have;
                    if (!ctx.frameAtLocked(head, have)) {
                        target = head;
                        needsRewind = head < ctx.nextIndex;
                        break;
                    }

                    const int ahead = ctx.nextIndex - head;
                    const bool full =
                        ctx.frames.size() >= static_cast<std::size_t>(kMaxCachedFrames)
                        || ctx.cachedBytes >= kMaxCachedBytes;
                    if (ahead >= kLookaheadFrames || full) {
                        // 播放头每前进一帧都会 wake：此处只等增量，不空转
                        ctx.wake.wait(&ctx.mutex, 100);
                        continue;
                    }
                    target = ctx.nextIndex;
                    break;
                }

                if (target < 0) {
                    // 状态机只会在 shutdown 时以 target < 0 退出（暂停走 wait 分支）。
                    // 本段全程持锁：startDecoding() 不可能在"看到 shutdown"与"回收
                    // 窗口"之间插入，因此不会误清正在复用的窗口。
                    if (ctx.shutdown.load(std::memory_order_acquire))
                        resetWindowLocked(ctx, doomed);
                    ctx.running.store(false, std::memory_order_release);
                    break;
                }

                if (needsRewind)
                    resetWindowLocked(ctx, doomed);
                decodeFrom = ctx.nextIndex;
            }

            // 回绕取出的旧帧在锁外释放（峰值内存不会叠加到重建 reader 期间）
            doomed.clear();

            // reader 只在需要时创建；回绕必须从 0 重建（Qt 动画 handler 无法随机寻址）。
            // 新建 reader 在锁外：顺序丢弃前缀可能耗时，不能阻塞 GUI 的 frameAt()/jumpToFrame()
            if (!reader || needsRewind) {
                reader.reset();
                PrefixOpenResult opened = openReaderAt(ctx.path, decodeFrom,
                                                       ctx.allocationLimitMB, target, ctx);
                if (opened.status == PrefixOpenStatus::Cancelled) {
                    // 取消的前缀不是 EOF、也不是真实帧数修正：丢掉 reader，
                    // 回到状态机重新读取最新的 requestedFrame/playhead
                    reader.reset();
                    continue;
                }
                reader = std::move(opened.reader);

                if (opened.status == PrefixOpenStatus::EndOfFile) {
                    // 重新打开后读到了 EOF（取消不会走到这里）。
                    // 前缀总是从文件头重新打开，因此 opened.decoded <= nextIndex。
                    // opened.decoded > 0 时按实际解码数修正真实帧数。
                    // opened.decoded == 0 表示此刻连第 0 帧都读不出来（文件被删除/
                    // 截断/独占），而不是"动画只有 0 帧"：不能覆盖已有修正值，且要
                    // 保留已解出的窗口，否则会把可显示的动画整个丢弃。
                    QMutexLocker locker(&ctx.mutex);
                    if (ctx.shutdown.load(std::memory_order_acquire)) {
                        resetWindowLocked(ctx, doomed);
                        ctx.running.store(false, std::memory_order_release);
                        break;
                    }
                    ctx.exhausted = true;
                    // opened.decoded == 0 表示本次从文件头读第 0 帧失败，不是
                    // “已确认 0 帧”；保留已有修正值，窗口仍按 nextIndex 保留。
                    const int actualDecoded = opened.decoded;
                    const int decoded =
                        actualDecoded > 0 ? actualDecoded : ctx.nextIndex;
                    trimWindowEndLocked(ctx, decoded, doomed);
                    ctx.nextIndex = decoded;
                    storeCorrectedFrameCount(ctx, actualDecoded);
                    ctx.running.store(false, std::memory_order_release);
                    // 与单帧 read() 的 EOF 路径一致：若挂起的跳转仍落在实际帧数内，
                    // 保持任务存活，下一轮从 0 回绕重解码
                    const int requested = ctx.requestedFrame.load(std::memory_order_acquire);
                    if (ctx.wantMore.load(std::memory_order_acquire)
                        && !ctx.shutdown.load(std::memory_order_acquire)
                        && requested >= 0 && requested < decoded) {
                        ctx.running.store(true, std::memory_order_release);
                        continue;
                    }
                    // 超出真实帧数的请求永远无法满足，清掉避免恢复时做一次
                    // 无意义的前缀定位（jumpToFrame 之后也会拒绝该帧号）
                    if (requested >= decoded)
                        ctx.requestedFrame.store(-1, std::memory_order_release);
                    break;
                }
            }

            // 前缀可能刚完成就遇到停止/暂停/换目标：本轮不 read()，回到状态机
            // 重新判断。reader 与 nextIndex 仍然对齐：暂停可原样保留，换目标时
            // 若需回绕会由上面的 needsRewind 分支丢弃重建。
            if (decodeCancelled(ctx, target))
                continue;

            // reader 独占于本任务，read() 在锁外：GUI 线程读缓存不会被解码阻塞
            QImage img = reader->read();
            const int delay = img.isNull() ? 0 : qMax(10, reader->nextImageDelay());

            QMutexLocker locker(&ctx.mutex);
            if (ctx.shutdown.load(std::memory_order_acquire)) {
                // 读取期间到达的停止请求：丢弃这一帧并回收整个窗口。
                // 停止后不得再把新帧写入缓存；大 QImage 在锁外析构。
                resetWindowLocked(ctx, doomed);
                ctx.running.store(false, std::memory_order_release);
                break;
            }

            if (img.isNull()) {
                // 实际帧数可能少于 imageCount() 的扫描值（损坏/截断的动画）；
                // imageCount() 未知（<=0）时以实际解出的帧数为准。
                // nextIndex 只由顺序成功解码推进，因此 decoded > 0 时就是真实帧数；
                // decoded == 0 表示第 0 帧读失败，不能写成“已确认 0 帧”。
                ctx.exhausted = true;
                const int decoded = ctx.nextIndex;
                storeCorrectedFrameCount(ctx, decoded);
                ctx.running.store(false, std::memory_order_release);
                // 循环播放回到已回收的首帧时会带着 requestedFrame 在此刻到达：
                // 保持任务存活，下一轮按请求 rewind 重解码，避免循环播放停住。
                // 请求号必须落在实际解出的范围内；超过 EOF（损坏/截断文件）时
                // 无法靠回绕满足，直接退出，由 jumpToFrame 的帧数上限拒绝后续重试。
                const int requested = ctx.requestedFrame.load(std::memory_order_acquire);
                if (ctx.wantMore.load(std::memory_order_acquire)
                    && !ctx.shutdown.load(std::memory_order_acquire)
                    && requested >= 0 && requested < decoded) {
                    ctx.running.store(true, std::memory_order_release);
                    continue;
                }
                // 超出真实帧数的请求永远无法满足，清掉避免恢复时白做前缀定位
                if (requested >= decoded)
                    ctx.requestedFrame.store(-1, std::memory_order_release);
                break;
            }

            // 顺序帧始终与 nextIndex/reader 位置一致，即使 read() 期间来了新跳转，
            // 发布它也不会错位；下一轮再消费 requestedFrame。停止请求已在上方拦截。
            const int index = ctx.nextIndex;
            auto frame = std::make_shared<const QImage>(std::move(img));
            ctx.cachedBytes += frame->sizeInBytes();
            ctx.frames.push_back({index, std::move(frame), delay});
            ctx.nextIndex = index + 1;

            trimLocked(ctx, ctx.playhead.load(std::memory_order_relaxed), doomed);
        }
    }

private:
    std::shared_ptr<ImageAnimated::DecodeContext> m_ctx;
};
} // namespace

ImageAnimated::ImageAnimated(std::unique_ptr<DocumentInfo> _info, int allocationLimitMB)
    : Image(std::move(_info))
{
    mSize = QSize(0, 0);
    loadWith(allocationLimitMB);
}

ImageAnimated::~ImageAnimated() {
    // 让后台任务在下一帧边界退出（它只持有 DecodeContext，不接触本对象）
    if (mCtx) {
        QMutexLocker locker(&mCtx->mutex);
        mCtx->wantMore.store(false, std::memory_order_relaxed);
        mCtx->shutdown.store(true, std::memory_order_relaxed);
        mCtx->wake.wakeAll();
    }
}

void ImageAnimated::loadWith(int allocationLimitMB) {
    if (isLoaded())
        return;

    loadMovie(allocationLimitMB);
    mLoaded = true;
}

void ImageAnimated::loadMovie(int allocationLimitMB) {
    if (mCtx)
        return;

    auto ctx = std::make_shared<DecodeContext>();
    ctx->path = mPath;
    ctx->allocationLimitMB = allocationLimitMB;

    // 首帧在加载线程用临时 reader 解码（面板/主窗口需要立即拿到尺寸）；
    // 后续帧由后台任务在自己的线程重建 reader 并定位，避免跨线程使用 QImageReader
    QImageReader reader(mPath);
    reader.setAllocationLimit(allocationLimitMB);
    if (!reader.canRead()) {
        mSize = QSize(0, 0);
        mFrameCount.store(0);
        return;
    }

    mFrameCount.store(qMax(0, reader.imageCount()));
    ctx->reportedFrameCount.store(mFrameCount.load(std::memory_order_relaxed),
                                  std::memory_order_relaxed);

    // 第 0 帧同步解码：其余帧在视图显示时交给后台预解码，GUI 线程不再同步 read()
    QImage first = reader.read();
    if (first.isNull()) {
        mSize = QSize(0, 0);
        mFrameCount.store(0);
        return;
    }

    auto firstFrame = std::make_shared<const QImage>(std::move(first));
    const int firstDelay = qMax(10, reader.nextImageDelay());
    {
        QMutexLocker locker(&ctx->mutex);
        ctx->frames.push_back({0, firstFrame, firstDelay});
        ctx->currentDelay = firstDelay;
        ctx->nextIndex = 1;
        ctx->cachedBytes = firstFrame->sizeInBytes();
    }

    mCtx = std::move(ctx);
    mSize = firstFrame->size();
    mCurrentFrame = 0;
    std::atomic_store_explicit(&cachedFrame, firstFrame, std::memory_order_release);

    // ⭐ 与 ImageStatic::loadGeneric 一致：解码期把文本元数据喂给 DocumentInfo，
    // 面板 getExifTags 不再在 GUI 线程兜底打开文件。
    // 用独立 reader：元数据查询不能干扰帧读取状态
    if (mDocInfo) {
        QImageReader metaReader(mPath);
        QHash<QString, QString> metadata;
        for (const QString &key : metaReader.textKeys()) {
            const QString value = metaReader.text(key);
            if (!value.isEmpty())
                metadata.insert(key, value);
        }
        mDocInfo->setCachedTextMetadata(std::move(metadata), mSize);
    }

    // ⭐ 此处不启动预解码：ImageAnimated 也可能只是被预加载/缓存（没有视图显示它），
    // 启动解码会为一个没人看的动画白跑满 CPU。真正的启动点是 viewer 的 showAnimation()
}

int ImageAnimated::frameCount() const {
    if (!mCtx)
        return 0;
    // EOF 后只有正数修正值才是可靠上限；0 按未知帧数处理（不是有效的
    // 修正值，旧状态也不应因此拒绝所有 jumpToFrame）。
    const int corrected = mCtx->correctedFrameCount.load(std::memory_order_acquire);
    if (corrected > 0)
        return corrected;
    return mFrameCount.load(std::memory_order_relaxed);
}

int ImageAnimated::currentFrameNumber() const {
    return mCurrentFrame;
}

int ImageAnimated::nextFrameDelay() const {
    if (!mCtx || mCurrentFrame < 0)
        return 0;
    QMutexLocker locker(&mCtx->mutex);
    return mCtx->currentDelay;
}

bool ImageAnimated::isValid() const {
    // mCtx 只在首帧成功解码后才会赋值。停止时帧窗口会被回收，但动画本身仍有效：
    // 切回时由后台重新解码首帧，不能因为窗口为空就拒绝显示。
    return mCtx != nullptr;
}

// 已解码帧（不触发解码）；未就绪返回 nullptr
std::shared_ptr<const QImage> ImageAnimated::frameAt(int index) const {
    if (!mCtx || index < 0)
        return nullptr;
    QMutexLocker locker(&mCtx->mutex);
    std::shared_ptr<const QImage> frame;
    mCtx->frameAtLocked(index, frame);
    return frame;
}

bool ImageAnimated::jumpToFrame(int frameNumber) {
    if (!mCtx || frameNumber < 0)
        return false;

    // 用 frameCount() 而不是 imageCount() 的原始扫描值：EOF 后修正值才是真实帧数，
    // 否则截断文件上跳到不存在的高帧号会反复触发回绕/重解码。
    const int count = frameCount();
    if (count > 0 && frameNumber >= count)
        return false;

    // 当前帧即使已被环形缓冲回收，cachedFrame 仍持有它，无需重新解码。
    // 顺带取消尚未被消费的旧跳转请求，避免拖回原帧后后台仍去追废弃目标
    if (frameNumber == mCurrentFrame) {
        QMutexLocker locker(&mCtx->mutex);
        mCtx->playhead.store(frameNumber, std::memory_order_relaxed);
        mCtx->requestedFrame.store(-1, std::memory_order_release);
        mCtx->wake.wakeOne();
        return true;
    }

    std::shared_ptr<const QImage> frame;
    int frameDelay = 10;
    bool startTask = false;
    FrameDeque doomed;   // 在锁外析构，避免大 QImage 释放阻塞 GUI
    {
        QMutexLocker locker(&mCtx->mutex);
        // 外层检查与取锁之间解码任务可能刚到达 EOF 并发布修正帧数：
        // 在锁内复查一次，避免为不存在的帧启动一次昂贵的顺序前缀定位。
        // 只有正数修正值才能作为可靠上限；0 表示未知，不能拒绝所有跳转。
        const int corrected = mCtx->correctedFrameCount.load(std::memory_order_acquire);
        if (corrected > 0 && frameNumber >= corrected)
            return false;
        if (!mCtx->frameAtLocked(frameNumber, frame, &frameDelay)) {
            // 请求后台补齐；播放头同步前移，前瞻窗口随之移动
            mCtx->wantMore.store(true, std::memory_order_relaxed);
            // 对象可能刚被 reset() 停过（shutdown=true）：清除退出标志，
            // 让仍在退出路径上的任务原地复活，而不是留下无人处理的请求
            mCtx->shutdown.store(false, std::memory_order_relaxed);
            mCtx->playhead.store(frameNumber, std::memory_order_relaxed);
            mCtx->requestedFrame.store(frameNumber, std::memory_order_release);
            // 已到 EOF（exhausted）时任务已退出：循环回到被回收的首帧需要重新拉起。
            // 与本段共用 mutex，不会与退出中的任务重复启动
            if (!mCtx->running.load(std::memory_order_relaxed)) {
                mCtx->running.store(true, std::memory_order_relaxed);
                startTask = true;
            }
            mCtx->wake.wakeOne();
            locker.unlock();
            if (startTask)
                frameDecodePool()->start(new FrameDecoderTask(mCtx));
            return false; // 仍在后台解码：交给调用方稍后重试
        }

        // 命中缓存：同步取消挂起的旧跳转，否则后台稍后会把播放头拖回废弃目标
        mCtx->requestedFrame.store(-1, std::memory_order_release);
        mCtx->playhead.store(frameNumber, std::memory_order_relaxed);
        mCtx->currentDelay = frameDelay;
        trimLocked(*mCtx, frameNumber, doomed);
        mCtx->wake.wakeOne();
    }

    mCurrentFrame = frameNumber;
    mCurrentPixmapValid = false;

    // 发布给 getImage() 消费者（打印/剪贴板等），仅一次 shared_ptr 原子交换
    std::atomic_store_explicit(&cachedFrame, frame, std::memory_order_release);
    return true;
}

bool ImageAnimated::jumpToNextFrame() {
    return jumpToFrame(mCurrentFrame + 1);
}

void ImageAnimated::startDecoding() {
    if (!mCtx)
        return;

    bool startTask = false;
    {
        QMutexLocker locker(&mCtx->mutex);
        mCtx->wantMore.store(true, std::memory_order_relaxed);
        // 允许 reset()/stopDecoding() 之后再次复用同一动画对象
        mCtx->shutdown.store(false, std::memory_order_relaxed);
        mCtx->wake.wakeOne();

        // 任务已在跑（含暂停挂起）时不要改播放头：
        // jumpToFrame 可能刚把跳转目标写进去
        if (!mCtx->running.load(std::memory_order_relaxed)) {
            const bool hasRequest =
                mCtx->requestedFrame.load(std::memory_order_relaxed) >= 0;
            if (!hasRequest)
                mCtx->playhead.store(qMax(0, mCurrentFrame), std::memory_order_relaxed);
            // 有挂起跳转时必须启动任务（即使已到 EOF，也要回绕重解码）
            if (!mCtx->exhausted || hasRequest) {
                mCtx->running.store(true, std::memory_order_relaxed);
                startTask = true;
            }
        }
    }

    if (startTask)
        frameDecodePool()->start(new FrameDecoderTask(mCtx));
}

void ImageAnimated::pauseDecoding() {
    if (!mCtx)
        return;
    QMutexLocker locker(&mCtx->mutex);
    // 只暂停推进：任务与 reader 保留，恢复时从当前位置继续
    mCtx->wantMore.store(false, std::memory_order_relaxed);
    mCtx->wake.wakeOne();
}

void ImageAnimated::stopDecoding() {
    if (!mCtx)
        return;

    FrameDeque doomed;   // 在锁外析构，避免大 QImage 释放阻塞 GUI/长时间持锁
    {
        QMutexLocker locker(&mCtx->mutex);
        mCtx->wantMore.store(false, std::memory_order_relaxed);
        mCtx->shutdown.store(true, std::memory_order_relaxed);
        mCtx->wake.wakeAll();

        // 没有解码任务在跑：其发布路径已经退出临界区，不会再写 frames，
        // 可以安全地立即回收窗口（large QImage 在锁外释放）。
        // 有任务在跑：由任务在实际退出前完成同样的清理，GUI 绝不等待 worker。
        if (!mCtx->running.load(std::memory_order_acquire))
            resetWindowLocked(*mCtx, doomed);
    }
}

bool ImageAnimated::isDecoding() const {
    return mCtx && mCtx->running.load(std::memory_order_acquire);
}

QPixmap ImageAnimated::currentPixmap(qreal devicePixelRatio) const {
    auto frame = frameAt(mCurrentFrame);
    // 环形缓冲可能已回收当前帧；cachedFrame 始终持有最近一次发布的帧
    if (!frame)
        frame = std::atomic_load_explicit(&cachedFrame, std::memory_order_acquire);
    if (!frame)
        return QPixmap();

    // 同一帧 + 同一 dpr 只做一次 QImage→QPixmap，循环播放时后续直接共享
    if (!mCurrentPixmapValid || mCurrentPixmap.devicePixelRatio() != devicePixelRatio) {
        mCurrentPixmap = QPixmap::fromImage(*frame);
        // dpr != 1 时打标记会 detach 一次：帧 QImage 仍被环形缓冲持有，无法 move 接管。
        // 但把它留在缓存里，updatePixmap 的同值设置即可提前返回，避免 GUI 线程每帧再拷一次。
        mCurrentPixmap.setDevicePixelRatio(devicePixelRatio);
        mCurrentPixmapValid = true;
    }
    return mCurrentPixmap;
}

bool ImageAnimated::save(QString destPath) {
    QFile file(mPath);
    if (!file.exists()) {
        return false;
    }

    if (!file.copy(destPath)) {
        return false;
    }

    if (destPath == this->filePath()) {
        mDocInfo->refresh();
    }
    return true;
}

bool ImageAnimated::save() {
    return false;
}

void ImageAnimated::getPixmap(QPixmap& outPixmap, qreal devicePixelRatio) const {
    // ✅ 优先使用当前帧的 QPixmap 缓存（同一帧只转换一次）
    outPixmap = currentPixmap(devicePixelRatio);
    if (!outPixmap.isNull())
        return;

    // fallback（极少发生）
    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    outPixmap = QPixmap(mPath, formatBytes.constData());
    if (!outPixmap.isNull())
        outPixmap.setDevicePixelRatio(devicePixelRatio);
}

std::shared_ptr<const QImage> ImageAnimated::getImage() const {
    return std::atomic_load_explicit(&cachedFrame, std::memory_order_acquire);
}

int ImageAnimated::height() const {
    return mSize.height();
}

int ImageAnimated::width() const {
    return mSize.width();
}

QSize ImageAnimated::size() const {
    return mSize;
}
