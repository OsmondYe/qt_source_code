#ifndef QLINEEDIT_P_H
#define QLINEEDIT_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include "private/qwidget_p.h"
#include "QtWidgets/qlineedit.h"
#if QT_CONFIG(toolbutton)
#include "QtWidgets/qtoolbutton.h"
#endif
#include "QtGui/qtextlayout.h"
#include "QtGui/qicon.h"
#include "QtWidgets/qstyleoption.h"
#include "QtCore/qbasictimer.h"
#if QT_CONFIG(completer)
#include "QtWidgets/qcompleter.h"
#endif
#include "QtCore/qpointer.h"
#include "QtCore/qmimedata.h"

#include "private/qwidgetlinecontrol_p.h"

#include <algorithm>


class QLineEditPrivate;

// QLineEditIconButton: This is a simple helper class that represents clickable icons that fade in with text
#if QT_CONFIG(toolbutton)
class Q_AUTOTEST_EXPORT QLineEditIconButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    explicit QLineEditIconButton(QWidget *parent =  0);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal value);
#ifndef QT_NO_ANIMATION
    void animateShow(bool visible) { startOpacityAnimation(visible ? 1.0 : 0.0); }
#endif

protected:
    void actionEvent(QActionEvent *e) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateCursor();

private:
#ifndef QT_NO_ANIMATION
    void startOpacityAnimation(qreal endValue);
#endif
    QLineEditPrivate *lineEditPrivate() const;

    qreal m_opacity;
};
#endif // QT_CONFIG(toolbutton)

class  QLineEditPrivate : public QWidgetPrivate
{
    //Q_DECLARE_PUBLIC(QLineEdit)
public:
    QWidgetLineControl *control;
#ifndef QT_NO_CONTEXTMENU
    QPointer<QAction> selectAllAction;
#endif

	
	QString placeholderText;	
	QPoint tripleClick;
    QBasicTimer tripleClickTimer;
    uint frame : 1;						// whether draws itself with a frame
    uint contextMenuEnabled : 1;
    uint cursorVisible : 1;
    uint dragEnabled : 1;
    uint clickCausedFocus : 1;
    int hscroll;
    int vscroll;
    uint alignment;
    QPoint mousePressPos;

	// 自己可以修改的margin
	int leftTextMargin; // use effectiveLeftTextMargin() in case of icon.
	int topTextMargin;
	int rightTextMargin; // use effectiveRightTextMargin() in case of icon.
	int bottomTextMargin;	
	
	// 控件本身的,有效显示部分和整个矩形之间还有预定义的间距
	static const int horizontalMargin;
	static const int verticalMargin;

private:
	SideWidgetEntryList leadingSideWidgets;
	SideWidgetEntryList trailingSideWidgets;
	int lastTextSize;


public:
    enum SideWidgetFlag {
        SideWidgetFadeInWithText = 0x1,
        SideWidgetCreatedByWidgetAction = 0x2,
        SideWidgetClearButton = 0x4
    };

    struct SideWidgetEntry {
        explicit SideWidgetEntry(QWidget *w = 0, QAction *a = 0, int _flags = 0) : widget(w), action(a), flags(_flags) {}

        QWidget *widget;
        QAction *action;
        int flags;
    };
    typedef std::vector<SideWidgetEntry> SideWidgetEntryList;

    struct SideWidgetParameters {
        int iconSize;
        int widgetWidth;
        int widgetHeight;
        int margin;
    };

    QLineEditPrivate()
        : control(0), frame(1), contextMenuEnabled(1), cursorVisible(0),
        dragEnabled(0), clickCausedFocus(0), hscroll(0), vscroll(0),
        alignment(Qt::AlignLeading | Qt::AlignVCenter),
        leftTextMargin(0), topTextMargin(0), rightTextMargin(0), bottomTextMargin(0),
        lastTextSize(0)
    {
    }

    ~QLineEditPrivate()    {    }

    void init(const QString&);

    QRect adjustedControlRect(const QRect &) const;

    int xToPos(int x, QTextLine::CursorPosition = QTextLine::CursorBetweenCharacters) const;
    bool inSelection(int x) const;
    QRect cursorRect() const;
    void setCursorVisible(bool visible);

    void updatePasswordEchoEditing(bool);

    void resetInputMethod();

    inline bool shouldEnableInputMethod() const
    {
        return !control->isReadOnly();
    }
    inline bool shouldShowPlaceholderText() const
    {
        return control->text().isEmpty() && control->preeditAreaText().isEmpty()
                && !((alignment & Qt::AlignHCenter) && q_func()->hasFocus());
    }

    static inline QLineEditPrivate *get(QLineEdit *lineEdit) {
        return lineEdit->d_func();
    }

    bool sendMouseEventToInputContext(QMouseEvent *e);

    QRect adjustedContentsRect() const;

    void _q_handleWindowActivate();
    void _q_textEdited(const QString &);
    void _q_cursorPositionChanged(int, int);
#ifdef QT_KEYPAD_NAVIGATION
    void _q_editFocusChange(bool);
#endif
    void _q_selectionChanged();
    void _q_updateNeeded(const QRect &);
#if QT_CONFIG(completer)
    void _q_completionHighlighted(const QString &);
#endif

#ifndef QT_NO_DRAGANDDROP
    QBasicTimer dndTimer;
    void drag();
#endif
    void _q_textChanged(const QString &);
    void _q_clearButtonClicked();

    QWidget *addAction(QAction *newAction, QAction *before, QLineEdit::ActionPosition, int flags = 0);
    void removeAction(QAction *action);
    SideWidgetParameters sideWidgetParameters() const;
    QIcon clearButtonIcon() const;
    void setClearButtonEnabled(bool enabled);
    void positionSideWidgets();
    inline bool hasSideWidgets() const { return !leadingSideWidgets.empty() || !trailingSideWidgets.empty(); }
    inline const SideWidgetEntryList &leftSideWidgetList() const
        { return q_func()->layoutDirection() == Qt::LeftToRight ? leadingSideWidgets : trailingSideWidgets; }
    inline const SideWidgetEntryList &rightSideWidgetList() const
        { return q_func()->layoutDirection() == Qt::LeftToRight ? trailingSideWidgets : leadingSideWidgets; }

    int effectiveLeftTextMargin() const;
    int effectiveRightTextMargin() const;

private:
    struct SideWidgetLocation {
        QLineEdit::ActionPosition position;
        int index;

        bool isValid() const { return index >= 0; }
    };
    friend class QTypeInfo<SideWidgetLocation>;

    SideWidgetLocation findSideWidget(const QAction *a) const;


};

//Q_DECLARE_TYPEINFO(QLineEditPrivate::SideWidgetEntry, Q_PRIMITIVE_TYPE);
//Q_DECLARE_TYPEINFO(QLineEditPrivate::SideWidgetLocation, Q_PRIMITIVE_TYPE);


#endif // QLINEEDIT_P_H
