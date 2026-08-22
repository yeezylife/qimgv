#include "shortcutbuilder.h"

#include <QStringList>
#include <utility>
#include <vector>

//------------------------------------------------------------------------------
QString ShortcutBuilder::fromEvent(QInputEvent *event) {
    if (!event)
        return {};

    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            return processKeyEvent(static_cast<QKeyEvent *>(event));

        case QEvent::Wheel:
            return processWheelEvent(static_cast<QWheelEvent *>(event));

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            return processMouseEvent(static_cast<QMouseEvent *>(event));

        default:
            return {};
    }
}

//------------------------------------------------------------------------------
QString ShortcutBuilder::processWheelEvent(QWheelEvent *event) {
    const QPoint delta = event->angleDelta();
    if (delta.isNull())
        return {};

    QString sequence;
    sequence.reserve(24);

    int y = delta.y();
    int x = delta.x();

    if (y != 0)
        sequence = (y < 0) ? "WheelDown" : "WheelUp";
    else
        sequence = (x < 0) ? "WheelDown" : "WheelUp";

    QString mods = modifierKeys(event);
    if (!mods.isEmpty())
        sequence.prepend(mods);

    return sequence;
}

//------------------------------------------------------------------------------
QString ShortcutBuilder::processMouseEvent(QMouseEvent *event) {
    QString sequence;

    switch (event->button()) {
        case Qt::LeftButton:   sequence = "LMB"; break;
        case Qt::RightButton:  sequence = "RMB"; break;
        case Qt::MiddleButton: sequence = "MiddleButton"; break;
        case Qt::XButton1:     sequence = "XButton1"; break;
        case Qt::XButton2:     sequence = "XButton2"; break;
        default: return {};
    }

    QString mods = modifierKeys(event);
    if (!mods.isEmpty())
        sequence.prepend(mods);

    const auto type = event->type();

    if (type == QEvent::MouseButtonDblClick) {
        sequence += "_DoubleClick";
        return sequence;
    }

    if ((type == QEvent::MouseButtonPress   && event->button() != Qt::RightButton) ||
        (type == QEvent::MouseButtonRelease && event->button() == Qt::RightButton))
    {
        return sequence;
    }

    return {};
}

//------------------------------------------------------------------------------
QString ShortcutBuilder::processKeyEvent(QKeyEvent *event) {
    if (event->type() != QEvent::KeyPress || isModifier(Qt::Key(event->key())))
        return {};

#if defined(__linux__) || defined(__FreeBSD__) || defined(_WIN32)
    return fromEventNativeScanCode(event);
#else
    return fromEventText(event);
#endif
}

//------------------------------------------------------------------------------
QString ShortcutBuilder::modifierKeys(QInputEvent *event) {
    if (!event)
        return {};

    // ⭐ 热路径（每次按键/滚轮/鼠标事件）：修饰键表进程内固定不变，
    // 预构建一次"按名称排序"的 (名称, 修饰符) 表，事件路径只做标志位测试与拼接，
    // 替代原先每次的 QHash 遍历 + QStringList 构造 + 排序 + join（多次堆分配）
    static const std::vector<std::pair<QString, Qt::KeyboardModifier>> sortedMods = []() {
        const auto &modsMap = inputMap->modifiers();
        // 先按原逻辑排序：QHash 迭代顺序受随机哈希种子影响，跨进程不稳定，
        // 排序保证生成的快捷键串与默认值(如 "Ctrl+Shift+S")错位时仍能匹配
        QStringList names;
        names.reserve(modsMap.size());
        for (auto it = modsMap.cbegin(); it != modsMap.cend(); ++it)
            names += it.key();
        names.sort();

        std::vector<std::pair<QString, Qt::KeyboardModifier>> table;
        table.reserve(names.size());
        for (const QString &name : names)
            table.emplace_back(name, modsMap.value(name));
        return table;
    }();

    const auto flags = event->modifiers();
    if (flags == Qt::NoModifier)
        return {};

    QString result;
    for (const auto &mod : sortedMods) {
        if (flags.testFlag(mod.second)) {
            result += mod.first;
            result += QLatin1Char('+');
        }
    }
    return result;
}

//------------------------------------------------------------------------------
bool ShortcutBuilder::isModifier(Qt::Key key) {
    switch (key) {
        case Qt::Key_Control:
        case Qt::Key_Super_L:
        case Qt::Key_Super_R:
        case Qt::Key_AltGr:
        case Qt::Key_Shift:
        case Qt::Key_Meta:
        case Qt::Key_Alt:
            return true;
        default:
            return false;
    }
}

//------------------------------------------------------------------------------
QString ShortcutBuilder::fromEventNativeScanCode(QKeyEvent *event) {
    QString sequence = inputMap->keyNameForScancode(event->nativeScanCode());
    if (sequence.isEmpty())
        return {};

    QString eventText = event->text();

    if (!eventText.isEmpty()) {
        const QChar keyChr = eventText.at(0);

        const bool useAltChr =
            (event->modifiers() == Qt::ShiftModifier) &&
            keyChr.isPrint() &&
            !keyChr.isLetter() &&
            !keyChr.isSpace();

        if (useAltChr)
            return eventText;
    }

    QString mods = modifierKeys(event);
    if (!mods.isEmpty())
        sequence.prepend(mods);

    return sequence;
}

//------------------------------------------------------------------------------
QString ShortcutBuilder::fromEventText(QKeyEvent *event) {
    QString sequence = QVariant::fromValue(Qt::Key(event->key())).toString();

    if (!sequence.isEmpty()) {
        sequence.remove(0, 4); // remove "Key_"

        if (sequence == "Return")
            sequence = "Enter";
        else if (sequence == "Escape")
            sequence = "Esc";
    } else {
        sequence = QKeySequence(event->key()).toString();
    }

    if (!sequence.isEmpty()) {
        QString mods = modifierKeys(event);
        if (!mods.isEmpty())
            sequence.prepend(mods);
    }

    return sequence;
}