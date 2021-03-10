class QStyleSheetStyle : public QWindowsStyle
{
    typedef QWindowsStyle ParentStyle;
    //Q_OBJECT;

public:
	QStyle *base;
    static int numinstances;
	int refcount;
	mutable QCss::Parser parser;
public:
	// 需要一个基本baseStyle,
    QStyleSheetStyle(QStyle *baseStyle);
    ~QStyleSheetStyle();

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = 0) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = 0) const override;
    void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const override;
    void drawItemText(QPainter *painter, const QRect& rect, int alignment, const QPalette &pal,
              bool enabled, const QString& text, QPalette::ColorRole textRole  = QPalette::NoRole) const override;
    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = 0) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *option) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = 0) const override;
    QRect itemPixmapRect(const QRect &rect, int alignment, const QPixmap &pixmap) const override;
    QRect itemTextRect(const QFontMetrics &metrics, const QRect &rect, int alignment, bool enabled,
                       const QString &text) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const override;
    void polish(QWidget *widget) override;
    void polish(QApplication *app) override;
    void polish(QPalette &pal) override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = 0) const override;
    QPalette standardPalette() const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *opt = 0,
                       const QWidget *widget = 0) const override;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option = 0,
                           const QWidget *w = 0 ) const override;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                          Qt::Orientation orientation, const QStyleOption *option = 0,
                          const QWidget *widget = 0) const override;
    int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = 0,
                  QStyleHintReturn *shret = 0) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = 0) const override;

    // These functions are called from QApplication/QWidget. Be careful.
    QStyle *baseStyle() const;
    void repolish(QWidget *widget);
    void repolish(QApplication *app);

    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

    
    void ref() { ++refcount; }
    void deref() { Q_ASSERT(refcount > 0); if (!--refcount) delete this; }
	// oye qss 自己的font设置特色
    void updateStyleSheetFont(QWidget* w) const;
    void saveWidgetFont(QWidget* w, const QFont& font) const;
    void clearWidgetFont(QWidget* w) const;

    bool styleSheetPalette(const QWidget* w, const QStyleOption* opt, QPalette* pal);

protected:
    bool event(QEvent *e) override;
private:    

    friend class QRenderRule;
    int nativeFrameWidth(const QWidget *);
    QRenderRule renderRule(const QObject *, int, quint64 = 0) const;
    QRenderRule renderRule(const QObject *, const QStyleOption *, int = 0) const;
    QSize defaultSize(const QWidget *, QSize, const QRect&, int) const;
    QRect positionRect(const QWidget *, const QRenderRule&, const QRenderRule&, int,
                       const QRect&, Qt::LayoutDirection) const;
    QRect positionRect(const QWidget *w, const QRenderRule &rule2, int pe,
                       const QRect &originRect, Qt::LayoutDirection dir) const;

    void setPalette(QWidget *);
    void unsetPalette(QWidget *);
    void setProperties(QWidget *);
    void setGeometry(QWidget *);
    void unsetStyleSheetFont(QWidget *) const;
    QVector<QCss::StyleRule> styleRules(const QObject *obj) const;
    bool hasStyleRule(const QObject *obj, int part) const;

    QHash<QStyle::SubControl, QRect> titleBarLayout(const QWidget *w, const QStyleOptionTitleBar *tb) const;

    QCss::StyleSheet getDefaultStyleSheet() const;

    static Qt::Alignment resolveAlignment(Qt::LayoutDirection, Qt::Alignment);
    static bool isNaturalChild(const QObject *obj);
    bool initObject(const QObject *obj) const;


private:
    //Q_DISABLE_COPY(QStyleSheetStyle)
    //Q_DECLARE_PRIVATE(QStyleSheetStyle)
};

class QStyleSheetStyleCaches : public QObject
{
    //Q_OBJECT;
public Q_SLOTS:
    void objectDestroyed(QObject *);
    void styleDestroyed(QObject *);
public:
    QHash<const QObject *, QVector<QCss::StyleRule> > styleRulesCache;
    QHash<const QObject *, QHash<int, bool> > hasStyleRuleCache;
    typedef QHash<int, QHash<quint64, QRenderRule> > QRenderRules;
    QHash<const QObject *, QRenderRules> renderRulesCache;
    QHash<const void *, QCss::StyleSheet> styleSheetCache; // parsed style sheets
    QSet<const QWidget *> autoFillDisabledWidgets;
    // widgets with whose palettes and fonts we have tampered:
    template <typename T>
    struct Tampered {
        T oldWidgetValue;
        uint resolveMask;

        // only call this function on an rvalue *this (it mangles oldWidgetValue)
        T reverted(T current)
        {
            oldWidgetValue.resolve(oldWidgetValue.resolve() & resolveMask);
            current.resolve(current.resolve() & ~resolveMask);
            current.resolve(oldWidgetValue);
            current.resolve(current.resolve() | oldWidgetValue.resolve());
            return current;
        }
    };
    QHash<const QWidget *, Tampered<QPalette>> customPaletteWidgets;
    QHash<const QWidget *, Tampered<QFont>> customFontWidgets;
};
template <typename T>
class QTypeInfo<QStyleSheetStyleCaches::Tampered<T>>
    : QTypeInfoMerger<QStyleSheetStyleCaches::Tampered<T>, T> {};

