#include "inputmap.h"

InputMap *inputMap = nullptr;

InputMap::InputMap() {
    initKeyMap();
    initModMap();
}

InputMap *InputMap::getInstance() {
    if(!inputMap) {
       inputMap = new InputMap();
    }
    return inputMap;
}

const QMap<quint32, QString> &InputMap::keys() {
    return keyMap;
}

const QMap<QString, Qt::KeyboardModifier> &InputMap::modifiers() {
    return modMap;
}

void InputMap::initKeyMap() {
    // key codes as reported by QKeyEvent::nativeScanCode()
    keyMap.clear();
#ifdef _WIN32
    // windows keymap for qimgv

    // Use initializer list for better performance
    keyMap = {
        // row 1
        {1, "Esc"},
        {59, "F1"}, {60, "F2"}, {61, "F3"}, {62, "F4"}, {63, "F5"},
        {64, "F6"}, {65, "F7"}, {66, "F8"}, {67, "F9"}, {68, "F10"},
        {87, "F11"}, {88, "F12"},
        {70, "ScrollLock"}, {69, "Pause"},

        // row 2
        {41, "`"}, {2, "1"}, {3, "2"}, {4, "3"}, {5, "4"}, {6, "5"},
        {7, "6"}, {8, "7"}, {9, "8"}, {10, "9"}, {11, "0"},
        {12, "-"}, {13, "="}, {14, "Backspace"},
        {338, "Ins"}, {327, "Home"}, {329, "PgUp"},

        // row 3
        {15, "Tab"}, {16, "Q"}, {17, "W"}, {18, "E"}, {19, "R"},
        {20, "T"}, {21, "Y"}, {22, "U"}, {23, "I"}, {24, "O"},
        {25, "P"}, {26, "["}, {27, "]"}, {28, "Enter"},
        {339, "Del"}, {335, "End"}, {337, "PgDown"},

        // row 4
        {58, "CapsLock"}, {30, "A"}, {31, "S"}, {32, "D"}, {33, "F"},
        {34, "G"}, {35, "H"}, {36, "J"}, {37, "K"}, {38, "L"},
        {39, ";"}, {40, "'"}, {43, "\\"},

        // row 5
        {44, "Z"}, {45, "X"}, {46, "C"}, {47, "V"}, {48, "B"},
        {49, "N"}, {50, "M"}, {51, ","}, {52, "."}, {53, "/"},
        {328, "Up"},

        // row 6
        {57, "Space"}, {349, "Menu"}, {331, "Left"}, {336, "Down"}, {333, "Right"},

        // numpad
        {325, "NumLock"}, {309, "/"}, {55, "*"}, {74, "-"},
        {71, "7"}, {72, "8"}, {73, "9"}, {78, "+"},
        {75, "4"}, {76, "5"}, {77, "6"},
        {79, "1"}, {80, "2"}, {81, "3"}, {284, "Enter"},
        {82, "0"}, {83, "."},

        // special
        {86, "<"}, // near left shift (iso layout)

        // looks like qt 6.7.0 changed nativeScanCode() values on windows
        // see https://github.com/easymodo/qimgv/issues/539
        {57426, "Ins"}, {57415, "Home"}, {57417, "PgUp"},
        {57427, "Del"}, {57423, "End"}, {57425, "PgDown"},
        {57416, "Up"}, {57437, "Menu"}, {57419, "Left"},
        {57424, "Down"}, {57421, "Right"},
        // numpad
        {57413, "NumLock"}, {57397, "/"}, {57372, "Enter"}
    };

#elif defined(__linux__) || defined(__FreeBSD__)
    // linux keymap for qimgv

    // Use initializer list for better performance
    keyMap = {
        // row 1
        {9, "Esc"}, {67, "F1"}, {68, "F2"}, {69, "F3"}, {70, "F4"},
        {71, "F5"}, {72, "F6"}, {73, "F7"}, {74, "F8"}, {75, "F9"},
        {76, "F10"}, {95, "F11"}, {96, "F12"},
        {107, "Print"}, {78, "ScrollLock"}, {127, "Pause"},

        // row 2
        {49, "`"}, {10, "1"}, {11, "2"}, {12, "3"}, {13, "4"},
        {14, "5"}, {15, "6"}, {16, "7"}, {17, "8"}, {18, "9"},
        {19, "0"}, {20, "-"}, {21, "="}, {22, "Backspace"},
        {118, "Ins"}, {110, "Home"}, {112, "PgUp"},

        // row 3
        {23, "Tab"}, {24, "Q"}, {25, "W"}, {26, "E"}, {27, "R"},
        {28, "T"}, {29, "Y"}, {30, "U"}, {31, "I"}, {32, "O"},
        {33, "P"}, {34, "["}, {35, "]"}, {36, "Enter"},
        {119, "Del"}, {115, "End"}, {117, "PgDown"},

        // row 4
        {66, "CapsLock"}, {38, "A"}, {39, "S"}, {40, "D"}, {41, "F"},
        {42, "G"}, {43, "H"}, {44, "J"}, {45, "K"}, {46, "L"},
        {47, ";"}, {48, "'"}, {51, "\\"},

        // row 5
        {52, "Z"}, {53, "X"}, {54, "C"}, {55, "V"}, {56, "B"},
        {57, "N"}, {58, "M"}, {59, ","}, {60, "."}, {61, "/"},
        {111, "Up"},

        // row 6
        {65, "Space"}, {135, "Menu"}, {113, "Left"}, {116, "Down"}, {114, "Right"},

        // numpad
        {77, "NumLock"}, {106, "/"}, {63, "*"}, {82, "-"},
        {79, "7"}, {80, "8"}, {81, "9"}, {86, "+"},
        {83, "4"}, {84, "5"}, {85, "6"},
        {87, "1"}, {88, "2"}, {89, "3"}, {104, "Enter"},
        {90, "0"}, {91, "."},

        // special
        {151, "Wake Up"}, // "Fn" key on thinkpad
        {94, "<"}, // near left shift (iso layout)
        {166, "PgBack"}, {167, "PgForward"}
    };
#endif
}

void InputMap::initModMap() {
    modMap.clear();
    modMap.insert(keyNameCtrl(),  Qt::ControlModifier);
    modMap.insert(keyNameAlt(),   Qt::AltModifier);
    modMap.insert(keyNameShift(), Qt::ShiftModifier);
}

QString InputMap::keyNameCtrl() {
#ifdef __APPLE__
    return "⌘";
#else
    return "Ctrl";
#endif
}

QString InputMap::keyNameAlt() {
#ifdef __APPLE__
    return "⌥";
#else
    return "Alt";
#endif
}

QString InputMap::keyNameShift() {
#ifdef __APPLE__
    return "⇧";
#else
    return "Shift";
#endif
}
