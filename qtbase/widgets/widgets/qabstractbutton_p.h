#ifndef QABSTRACTBUTTON_P_H
#define QABSTRACTBUTTON_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qabstractbutton.h"

#include "QtCore/qbasictimer.h"
#include "private/qwidget_p.h"



class QAbstractButtonPrivate : public QWidgetPrivate
{
		
public:
    QAbstractButtonPrivate(QSizePolicy::ControlType type = QSizePolicy::DefaultType);

	    //Q_DECLARE_PUBLIC(QAbstractButton)
    QAbstractButton* q_func() { return static_cast<QAbstractButton *>(q_ptr); }
	

    QString text;
    QIcon icon;
    QSize iconSize;
	
    QKeySequence shortcut;
    int shortcutId;

    uint checkable :1;
    uint checked :1;
    uint autoRepeat :1;
    uint autoExclusive :1;
    uint down :1;
    uint blockRefresh :1;
    uint pressed : 1;

    QButtonGroup* group;
    QBasicTimer repeatTimer;
    QBasicTimer animateTimer;

    int autoRepeatDelay, autoRepeatInterval;

    QSizePolicy::ControlType controlType;
    mutable QSize sizeHint;

    void init();
    void click();
    void refresh();

    QList<QAbstractButton *>queryButtonList() const;
    QAbstractButton *queryCheckedButton() const;
    void notifyChecked();
    void moveFocus(int key);
    void fixFocusPolicy();

    void emitPressed();
    void emitReleased();
    void emitClicked();
    void emitToggled(bool checked);


};


#endif // QABSTRACTBUTTON_P_H
