#ifndef QPALETTE_H
#define QPALETTE_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qwindowdefs.h>
#include <QtGui/qcolor.h>
#include <QtGui/qbrush.h>

QT_BEGIN_NAMESPACE


class QPalettePrivate;
class QVariant;

class  QPalette
{
    //Q_GADGET
public:
	// 作为重要的颜色系统, 调色板提供了常见的场景下元素的颜色
	inline ColorGroup currentColorGroup() const { return static_cast<ColorGroup>(data.current_group); }
    inline const QColor &color(ColorGroup cg, ColorRole cr) const    { return brush(cg, cr).color(); }
	inline const QColor &color(ColorRole cr) const { return color(Current, cr); }
    inline const QBrush &brush(ColorRole cr) const { return brush(Current, cr); }
    inline const QBrush &foreground() const { return brush(WindowText); }
    inline const QBrush &windowText() const { return brush(WindowText); }
    inline const QBrush &button() const { return brush(Button); }
    inline const QBrush &light() const { return brush(Light); }
    inline const QBrush &dark() const { return brush(Dark); }
    inline const QBrush &mid() const { return brush(Mid); }
    inline const QBrush &text() const { return brush(Text); }
    inline const QBrush &base() const { return brush(Base); }
    inline const QBrush &alternateBase() const { return brush(AlternateBase); }
    inline const QBrush &toolTipBase() const { return brush(ToolTipBase); }
    inline const QBrush &toolTipText() const { return brush(ToolTipText); }
    inline const QBrush &background() const { return brush(Window); }
    inline const QBrush &window() const { return brush(Window); }
    inline const QBrush &midlight() const { return brush(Midlight); }
    inline const QBrush &brightText() const { return brush(BrightText); }
    inline const QBrush &buttonText() const { return brush(ButtonText); }
    inline const QBrush &shadow() const { return brush(Shadow); }
    inline const QBrush &highlight() const { return brush(Highlight); }
    inline const QBrush &highlightedText() const { return brush(HighlightedText); }
    inline const QBrush &link() const { return brush(Link); }
    inline const QBrush &linkVisited() const { return brush(LinkVisited); }

public:
    QPalette();
    QPalette(const QColor &button);
    QPalette(Qt::GlobalColor button);
    QPalette(const QColor &button, const QColor &window);
    QPalette(const QBrush &windowText, const QBrush &button, const QBrush &light,
             const QBrush &dark, const QBrush &mid, const QBrush &text,
             const QBrush &bright_text, const QBrush &base, const QBrush &window);
    QPalette(const QColor &windowText, const QColor &window, const QColor &light,
             const QColor &dark, const QColor &mid, const QColor &text, const QColor &base);
    QPalette(const QPalette &palette);
    ~QPalette();
	
    operator QVariant() const;

    // Do not change the order, the serialization format depends on it
    enum ColorGroup { Active, Disabled, Inactive, NColorGroups, Current, All, Normal = Active };
	
    enum ColorRole { WindowText, Button, Light, Midlight, Dark, Mid,
                     Text, BrightText, ButtonText, Base, Window, Shadow,
                     Highlight, HighlightedText,
                     Link, LinkVisited,
                     AlternateBase,
                     NoRole,
                     ToolTipBase, ToolTipText,
                     NColorRoles = ToolTipText + 1,
                     Foreground = WindowText, Background = Window
                   };


    
    inline void setCurrentColorGroup(ColorGroup cg) { data.current_group = cg; }


	
    const QBrush &brush(ColorGroup cg, ColorRole cr) const;
    inline void setColor(ColorGroup cg, ColorRole cr, const QColor &color);
    inline void setColor(ColorRole cr, const QColor &color);
    inline void setBrush(ColorRole cr, const QBrush &brush);
    bool isBrushSet(ColorGroup cg, ColorRole cr) const;
    void setBrush(ColorGroup cg, ColorRole cr, const QBrush &brush);
    void setColorGroup(ColorGroup cr, const QBrush &windowText, const QBrush &button,
                       const QBrush &light, const QBrush &dark, const QBrush &mid,
                       const QBrush &text, const QBrush &bright_text, const QBrush &base,
                       const QBrush &window);
    bool isEqual(ColorGroup cr1, ColorGroup cr2) const;

    bool operator==(const QPalette &p) const;
    inline bool operator!=(const QPalette &p) const { return !(operator==(p)); }
    bool isCopyOf(const QPalette &p) const;

    qint64 cacheKey() const;

    QPalette resolve(const QPalette &) const;
    inline uint resolve() const { return data.resolve_mask; }
    inline void resolve(uint mask) { data.resolve_mask = mask; }

private:
    void setColorGroup(ColorGroup cr, const QBrush &windowText, const QBrush &button,
                       const QBrush &light, const QBrush &dark, const QBrush &mid,
                       const QBrush &text, const QBrush &bright_text,
                       const QBrush &base, const QBrush &alternate_base,
                       const QBrush &window, const QBrush &midlight,
                       const QBrush &button_text, const QBrush &shadow,
                       const QBrush &highlight, const QBrush &highlighted_text,
                       const QBrush &link, const QBrush &link_visited);
    void setColorGroup(ColorGroup cr, const QBrush &windowText, const QBrush &button,
                       const QBrush &light, const QBrush &dark, const QBrush &mid,
                       const QBrush &text, const QBrush &bright_text,
                       const QBrush &base, const QBrush &alternate_base,
                       const QBrush &window, const QBrush &midlight,
                       const QBrush &button_text, const QBrush &shadow,
                       const QBrush &highlight, const QBrush &highlighted_text,
                       const QBrush &link, const QBrush &link_visited,
                       const QBrush &toolTipBase, const QBrush &toolTipText);
    void init();
    void detach();

    QPalettePrivate *d;
    struct Data {
        uint current_group : 4;
        uint resolve_mask : 28;
    };
    union {
        Data data;
        quint32 for_faster_swapping_dont_use;
    };
    
};

inline void QPalette::setColor(ColorGroup acg, ColorRole acr,
                               const QColor &acolor)
{ setBrush(acg, acr, QBrush(acolor)); }
inline void QPalette::setColor(ColorRole acr, const QColor &acolor)
{ setColor(All, acr, acolor); }
inline void QPalette::setBrush(ColorRole acr, const QBrush &abrush)
{ setBrush(All, acr, abrush); }



QT_END_NAMESPACE

#endif // QPALETTE_H
