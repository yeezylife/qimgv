#include "shortcutbuilder.h"

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

    const auto &modsMap = inputMap->modifiers();
    const auto flags = event->modifiers();

    QString result;
    result.reserve(16);

    for (auto it = modsMap.cbegin(); it != modsMap.cend(); ++it) {
        if (flags.testFlag(it.value())) {
            result += it.key();
            result += '+';
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
    QString sequence = inputMap->keys().value(event->nativeScanCode());
    if (sequence.isEmpty())
        return {};

    const QString eventText = event->text();

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