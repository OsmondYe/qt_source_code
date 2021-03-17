#ifndef QPIXMAPSTYLE_H
#define QPIXMAPSTYLE_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/QCommonStyle>
#include <QtWidgets/QTileRules>


QT_BEGIN_NAMESPACE

class QPixmapStylePrivate;

class QPixmapStyle : public QCommonStyle
{
    //Q_OBJECT

public:
    enum ControlDescriptor {
        BG_Background,
        LE_Enabled,             // QLineEdit
        LE_Disabled,
        LE_Focused,
        PB_Enabled,             // QPushButton
        PB_Pressed,
        PB_PressedDisabled,
        PB_Checked,
        PB_Disabled,
        TE_Enabled,             // QTextEdit
        TE_Disabled,
        TE_Focused,
        PB_HBackground,         // Horizontal QProgressBar
        PB_HContent,
        PB_HComplete,
        PB_VBackground,         // Vertical QProgressBar
        PB_VContent,
        PB_VComplete,
        SG_HEnabled,            // Horizontal QSlider groove
        SG_HDisabled,
        SG_HActiveEnabled,
        SG_HActivePressed,
        SG_HActiveDisabled,
        SG_VEnabled,            // Vertical QSlider groove
        SG_VDisabled,
        SG_VActiveEnabled,
        SG_VActivePressed,
        SG_VActiveDisabled,
        DD_ButtonEnabled,       // QComboBox (DropDown)
        DD_ButtonDisabled,
        DD_ButtonPressed,
        DD_PopupDown,
        DD_PopupUp,
        DD_ItemSelected,
        ID_Selected,            // QStyledItemDelegate
        SB_Horizontal,          // QScrollBar
        SB_Vertical
    };

    enum ControlPixmap {
        CB_Enabled,             // QCheckBox
        CB_Checked,
        CB_Pressed,
        CB_PressedChecked,
        CB_Disabled,
        CB_DisabledChecked,
        RB_Enabled,             // QRadioButton
        RB_Checked,
        RB_Pressed,
        RB_Disabled,
        RB_DisabledChecked,
        SH_HEnabled,            // Horizontal QSlider handle
        SH_HDisabled,
        SH_HPressed,
        SH_VEnabled,            // Vertical QSlider handle
        SH_VDisabled,
        SH_VPressed,
        DD_ArrowEnabled,        // QComboBox (DropDown) arrow
        DD_ArrowDisabled,
        DD_ArrowPressed,
        DD_ArrowOpen,
        DD_ItemSeparator,
        ID_Separator            // QStyledItemDelegate separator
    };

public:
    QPixmapStyle();
    ~QPixmapStyle();

    void polish(QApplication *application) Q_DECL_OVERRIDE;
    void polish(QPalette &palette) Q_DECL_OVERRIDE;
    void polish(QWidget *widget) Q_DECL_OVERRIDE;
    void unpolish(QApplication *application) Q_DECL_OVERRIDE;
    void unpolish(QWidget *widget) Q_DECL_OVERRIDE;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
            QPainter *painter, const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;
    void drawControl(ControlElement element, const QStyleOption *option,
            QPainter *painter, const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget=0) const Q_DECL_OVERRIDE;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
            const QSize &contentsSize, const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;
    QRect subElementRect(SubElement element, const QStyleOption *option,
            const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *option,
                         SubControl sc, const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = Q_NULLPTR,
            const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;
    int styleHint(StyleHint hint, const QStyleOption *option,
                  const QWidget *widget, QStyleHintReturn *returnData) const Q_DECL_OVERRIDE;
    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                     const QPoint &pos, const QWidget *widget) const Q_DECL_OVERRIDE;

    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;

    void addDescriptor(ControlDescriptor control, const QString &fileName,
                       QMargins margins = QMargins(),
                       QTileRules tileRules = QTileRules(Qt::RepeatTile, Qt::RepeatTile));
    void copyDescriptor(ControlDescriptor source, ControlDescriptor dest);
    void drawCachedPixmap(ControlDescriptor control, const QRect &rect, QPainter *p) const;

    void addPixmap(ControlPixmap control, const QString &fileName,
                   QMargins margins = QMargins());
    void copyPixmap(ControlPixmap source, ControlPixmap dest);

protected:
    void drawPushButton(const QStyleOption *option,
                        QPainter *painter, const QWidget *widget) const;
    void drawLineEdit(const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawTextEdit(const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawCheckBox(const QStyleOption *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawRadioButton(const QStyleOption *option,
                         QPainter *painter, const QWidget *widget) const;
    void drawPanelItemViewItem(const QStyleOption *option,
                               QPainter *painter, const QWidget *widget) const;
    void drawProgressBarBackground(const QStyleOption *option,
                                   QPainter *painter, const QWidget *widget) const;
    void drawProgressBarLabel(const QStyleOption *option,
                              QPainter *painter, const QWidget *widget) const;
    void drawProgressBarFill(const QStyleOption *option,
                             QPainter *painter, const QWidget *widget) const;
    void drawSlider(const QStyleOptionComplex *option,
                    QPainter *painter, const QWidget *widget) const;
    void drawComboBox(const QStyleOptionComplex *option,
                      QPainter *painter, const QWidget *widget) const;
    void drawScrollBar(const QStyleOptionComplex *option,
                       QPainter *painter, const QWidget *widget) const;

    QSize pushButtonSizeFromContents(const QStyleOption *option,
                                     const QSize &contentsSize, const QWidget *widget) const;
    QSize lineEditSizeFromContents(const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget) const;
    QSize progressBarSizeFromContents(const QStyleOption *option,
                                      const QSize &contentsSize, const QWidget *widget) const;
    QSize sliderSizeFromContents(const QStyleOption *option,
                                 const QSize &contentsSize, const QWidget *widget) const;
    QSize comboBoxSizeFromContents(const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget) const;
    QSize itemViewSizeFromContents(const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget) const;

    QRect comboBoxSubControlRect(const QStyleOptionComplex *option, QPixmapStyle::SubControl sc,
                                 const QWidget *widget) const;
    QRect scrollBarSubControlRect(const QStyleOptionComplex *option, QPixmapStyle::SubControl sc,
                                  const QWidget *widget) const;

protected:
    QPixmapStyle(QPixmapStylePrivate &dd);

private:
    Q_DECLARE_PRIVATE(QPixmapStyle)
};

QT_END_NAMESPACE

#endif // QPIXMAPSTYLE_H
