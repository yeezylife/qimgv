#include "themestore.h"

// Helper function to create QColor from RGB hex value efficiently
// Avoids runtime string parsing overhead of QColor("#RRGGBB")
static inline QColor fromHex(uint rgb) {
    return QColor::fromRgb(rgb);
}

ColorScheme ThemeStore::colorScheme(ColorSchemes name) {
    // Optimization: Cache static themes (Light, Black, Dark, DarkBlue)
    // These are computed only once due to 'static' keyword.
    static const ColorScheme lightScheme = []() {
        BaseColorScheme base;
        base.tid = static_cast<int>(COLORS_LIGHT);
        base.accent = fromHex(0xff719ccd);
        base.background = fromHex(0xff1a1a1a);
        base.background_fullscreen = fromHex(0xff1a1a1a);
        base.icons = fromHex(0xff656768);
        base.overlay = fromHex(0xff1a1a1a);
        base.overlay_text = fromHex(0xffd2d2d2);
        base.text = fromHex(0xff353535);
        base.scrollbar = fromHex(0xffaaaaaa);
        base.widget = fromHex(0xffffffff);
        base.widget_border = fromHex(0xffc3c3c3);
        return ColorScheme(base);
    }();

    static const ColorScheme darkBlueScheme = []() {
        BaseColorScheme base;
        base.tid = static_cast<int>(COLORS_DARKBLUE);
        base.background = fromHex(0xff18191a);
        base.background_fullscreen = fromHex(0xff18191a);
        base.text = fromHex(0xffcdd2d7);
        base.icons = fromHex(0xffbabec3);
        base.widget = fromHex(0xff232629);
        base.widget_border = fromHex(0xff26292d);
        base.accent = fromHex(0xff336ca5);
        base.scrollbar = fromHex(0xff4f565c);
        base.overlay_text = fromHex(0xffd2d2d2);
        base.overlay = fromHex(0xff1a1a1a);
        return ColorScheme(base);
    }();

    static const ColorScheme blackScheme = []() {
        BaseColorScheme base;
        base.tid = static_cast<int>(COLORS_BLACK);
        base.background = fromHex(0xff000000);
        base.background_fullscreen = fromHex(0xff000000);
        base.text = fromHex(0xffb0b0b0);
        base.icons = fromHex(0xff999999);
        base.widget = fromHex(0xff080808);
        base.widget_border = fromHex(0xff181818);
        base.accent = fromHex(0xff5a5a5a);
        base.scrollbar = fromHex(0xff343434);
        base.overlay_text = fromHex(0xff999999);
        base.overlay = fromHex(0xff000000);
        return ColorScheme(base);
    }();

    static const ColorScheme darkScheme = []() {
        BaseColorScheme base;
        base.tid = static_cast<int>(COLORS_DARK);
        base.background = fromHex(0xff1a1a1a);
        base.background_fullscreen = fromHex(0xff1a1a1a);
        base.text = fromHex(0xffb6b6b6);
        base.icons = fromHex(0xffa4a4a4);
        base.widget = fromHex(0xff252525);
        base.widget_border = fromHex(0xff2c2c2c);
        base.accent = fromHex(0xff8c9b81);
        base.scrollbar = fromHex(0xff5a5a5a);
        base.overlay_text = fromHex(0xffd2d2d2);
        base.overlay = fromHex(0xff1a1a1a);
        return ColorScheme(base);
    }();

    // Handle dynamic themes (System, Customized)
    if (name == COLORS_SYSTEM || name == COLORS_CUSTOMIZED) {
        // Optimization: Construct QPalette only when needed
        QPalette p;
        BaseColorScheme base = {};

        base.background = p.window().color();
        base.background_fullscreen = p.window().color();
        base.widget = p.window().color();
        base.widget_border = p.window().color();
        base.text = p.text().color();
        base.icons = p.text().color();
        base.accent = p.highlight().color();
        
        // Optimization: Pre-calculate highlight color once
        QColor highlight = p.highlight().color();
        base.scrollbar.setHsv(highlight.hue(), 
                              qBound(0, highlight.saturation() - 20, 240), 
                              qBound(0, highlight.value() - 35, 240));
        
        base.tid = static_cast<int>(name);
        return ColorScheme(base);
    }

    // Return cached schemes (copy is cheap due to implicit sharing of QColor)
    switch(name) {
        case COLORS_LIGHT:       return lightScheme;
        case COLORS_DARKBLUE:    return darkBlueScheme;
        case COLORS_BLACK:       return blackScheme;
        case COLORS_DARK:        return darkScheme;
        default:                 return ColorScheme(); // Should not be reached
    }
}

//---------------------------------------------------------------------

ColorScheme::ColorScheme()
    : tid(-1)
{
    // QColor members are default constructed to invalid colors automatically
}

ColorScheme::ColorScheme(BaseColorScheme base) {
    setBaseColors(base);
}

void ColorScheme::setBaseColors(BaseColorScheme base) {
    background = base.background;
    background_fullscreen = base.background_fullscreen;
    text = base.text;
    icons = base.icons;
    widget = base.widget;
    widget_border = base.widget_border;
    accent = base.accent;
    overlay = base.overlay;
    overlay_text = base.overlay_text;
    scrollbar = base.scrollbar;
    tid = base.tid;

    createColorVariants();
}

void ColorScheme::createColorVariants() {
    // Optimization: Cache frequently accessed values to avoid repeated function calls
    const int widgetValue = widget.value();
    const qreal widgetValueF = widget.valueF();

    if(widgetValueF <= 0.45f) { // dark theme
        // regular buttons - from widget bg
        // Optimization: widgetValue is correct here
        button.setHsv(widget.hue(), widget.saturation(), qMin(widgetValue + 21, 255));
        
        // Optimization: Remove redundant QColor(...) constructor calls
        button_hover = button.lighter(112);
        button_pressed = button.darker(112);
        
        scrollbar_hover = scrollbar.lighter(120);

        // text
        text_hc = text.lighter(110);
        text_hc2 = text.lighter(118);
        text_lc = text.darker(115);
        text_lc2 = text.darker(160);
    } else { // light theme
        // regular buttons - from widget bg
        button.setHsv(widget.hue(), widget.saturation(), qMax(widgetValue - 42, 0));
        
        button_hover = button.darker(106);
        button_pressed = button.darker(118);
        
        scrollbar_hover = scrollbar.darker(120);

        // text
        text_hc = text.darker(104);
        text_hc2 = text.darker(112);
        text_lc = text.lighter(130);
        text_lc2 = text.lighter(160);
    }

    // misc
    input_field_focus = accent;
}
