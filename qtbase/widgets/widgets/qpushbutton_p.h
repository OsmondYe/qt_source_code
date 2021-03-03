#ifndef QPUSHBUTTON_P_H
#define QPUSHBUTTON_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qabstractbutton_p.h"

QT_REQUIRE_CONFIG(pushbutton);

QT_BEGIN_NAMESPACE

class QDialog;
class QPushButton;

class QPushButtonPrivate : public QAbstractButtonPrivate
{
    //Q_DECLARE_PUBLIC(QPushButton)
public:
    QPointer<QMenu> menu;
    uint autoDefault : 2;
    uint defaultButton : 1;
    uint flat : 1;
    uint menuOpen : 1;
    mutable uint lastAutoDefault : 1;
public:
    enum AutoDefaultValue { Off = 0, On = 1, Auto = 2 };

    QPushButtonPrivate()
        : QAbstractButtonPrivate(QSizePolicy::PushButton), autoDefault(Auto),
          defaultButton(false), flat(false), menuOpen(false), lastAutoDefault(false) {}

    inline void init() { resetLayoutItemMargins(); }
    static QPushButtonPrivate* get(QPushButton *b) { return b->d_func(); }

#if QT_CONFIG(menu)
    QPoint adjustedMenuPosition();
#endif
    void resetLayoutItemMargins();
    void _q_popupPressed();
    QDialog *dialogParent() const;



};

QT_END_NAMESPACE

#endif // QPUSHBUTTON_P_H
