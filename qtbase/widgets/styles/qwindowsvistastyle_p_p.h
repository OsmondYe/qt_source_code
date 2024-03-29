#ifndef QWINDOWSVISTASTYLE_P_P_H
#define QWINDOWSVISTASTYLE_P_P_H


#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qwindowsvistastyle_p.h"

#if QT_CONFIG(style_windowsvista)
#include <private/qwindowsxpstyle_p_p.h>
#include <private/qstyleanimation_p.h>
#include <private/qpaintengine_raster_p.h>
#include <qpaintengine.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qpixmapcache.h>
#include <qstyleoption.h>
#if QT_CONFIG(pushbutton)
#include <qpushbutton.h>
#endif
#include <qradiobutton.h>
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#include <qgroupbox.h>
#if QT_CONFIG(toolbutton)
#include <qtoolbutton.h>
#endif
#if QT_CONFIG(spinbox)
#include <qspinbox.h>
#endif
#include <qtoolbar.h>
#if QT_CONFIG(combobox)
#include <qcombobox.h>
#endif
#if QT_CONFIG(scrollbar)
#include <qscrollbar.h>
#endif
#if QT_CONFIG(progressbar)
#include <qprogressbar.h>
#endif
#if QT_CONFIG(dockwidget)
#include <qdockwidget.h>
#endif
#if QT_CONFIG(listview)
#include <qlistview.h>
#endif
#if QT_CONFIG(treeview)
#include <qtreeview.h>
#endif
#include <qtextedit.h>
#include <qmessagebox.h>
#if QT_CONFIG(dialogbuttonbox)
#include <qdialogbuttonbox.h>
#endif
#include <qinputdialog.h>
#if QT_CONFIG(tableview)
#include <qtableview.h>
#endif
#include <qdatetime.h>
#include <qcommandlinkbutton.h>

QT_BEGIN_NAMESPACE

#if !defined(SCHEMA_VERIFY_VSSYM32)
#define TMT_ANIMATIONDURATION       5006
#define TMT_TRANSITIONDURATIONS     6000
#define EP_EDITBORDER_NOSCROLL      6
#define EP_EDITBORDER_HVSCROLL      9
#define EP_BACKGROUND               3
#define EBS_NORMAL                  1
#define EBS_HOT                     2
#define EBS_DISABLED                3
#define EBS_READONLY                5
#define PBS_DEFAULTED_ANIMATING     6
#define MBI_NORMAL                  1
#define MBI_HOT                     2
#define MBI_PUSHED                  3
#define MBI_DISABLED                4
#define MB_ACTIVE                   1
#define MB_INACTIVE                 2
#define PP_FILL                     5
#define PP_FILLVERT                 6
#define PP_MOVEOVERLAY              8
#define PP_MOVEOVERLAYVERT          10
#define MENU_BARBACKGROUND          7
#define MENU_BARITEM                8
#define MENU_POPUPCHECK             11
#define MENU_POPUPCHECKBACKGROUND   12
#define MENU_POPUPGUTTER            13
#define MENU_POPUPITEM              14
#define MENU_POPUPBORDERS           10
#define MENU_POPUPSEPARATOR         15
#define MC_CHECKMARKNORMAL          1
#define MC_CHECKMARKDISABLED        2
#define MC_BULLETNORMAL             3
#define MC_BULLETDISABLED           4
#define ABS_UPHOVER                 17
#define ABS_DOWNHOVER               18
#define ABS_LEFTHOVER               19
#define ABS_RIGHTHOVER              20
#define CP_DROPDOWNBUTTONRIGHT      6
#define CP_DROPDOWNBUTTONLEFT       7
#define SCRBS_HOVER                 5
#define TVP_HOTGLYPH                4
#define SPI_GETCLIENTAREAANIMATION  0x1042
#define TDLG_PRIMARYPANEL           1
#define TDLG_SECONDARYPANEL         8
#endif

class QWindowsVistaAnimation : public QBlendStyleAnimation
{
    Q_OBJECT
public:
    QWindowsVistaAnimation(Type type, QObject *target) : QBlendStyleAnimation(type, target) { }

    virtual bool isUpdateNeeded() const;
    void paint(QPainter *painter, const QStyleOption *option);
};


// Handles state transition animations
class QWindowsVistaTransition : public QWindowsVistaAnimation
{
    Q_OBJECT
public:
    QWindowsVistaTransition(QObject *target) : QWindowsVistaAnimation(Transition, target) {}
};


// Handles pulse animations (default buttons)
class QWindowsVistaPulse: public QWindowsVistaAnimation
{
    Q_OBJECT
public:
    QWindowsVistaPulse(QObject *target) : QWindowsVistaAnimation(Pulse, target) {}
};


class QWindowsVistaStylePrivate :  public QWindowsXPStylePrivate
{
    Q_DECLARE_PUBLIC(QWindowsVistaStyle)

public:
    QWindowsVistaStylePrivate();

    static int fixedPixelMetric(QStyle::PixelMetric pm);
    static inline bool useVista();
    bool transitionsEnabled() const;
};

QT_END_NAMESPACE

#endif // style_windowsvista

#endif // QWINDOWSVISTASTYLE_P_P_H
