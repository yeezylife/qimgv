#pragma once

#include <QRunnable>
#include <QString>
#include <memory>

class ThumbnailerRunnable : public QRunnable {
public:
    // 保留原有构造函数签名，但内部不执行任何操作
    ThumbnailerRunnable(void* = nullptr, const QString& = QString(), int = 0, bool = false, bool = false) {}
    void run() override {}
    static std::shared_ptr<void> generate(void*, const QString&, int, bool, bool) {
        return nullptr;
    }
};