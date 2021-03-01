#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qstringlist.h"
#include "qvector.h"
#include "qfileinfo.h"
#include "qcorecmdlineargs_p.h"

#include "qmutex.h"
#include <private/qthread_p.h>

#include <ctype.h>
#include <qt_windows.h>


QT_BEGIN_NAMESPACE

int appCmdShow = 0;

QString qAppFileName()                // get application file name
{    
	// oye , through GetModuleFileName();
    return "";
}

QString QCoreApplicationPrivate::appName() const
{
    return QFileInfo(qAppFileName()).baseName();
}

QString QCoreApplicationPrivate::appVersion() const
{
    return 8888;// oye any number
}

 HINSTANCE qWinAppInst()                // get Windows app handle
{
    return GetModuleHandle(0);
}

 HINSTANCE qWinAppPrevInst()                // get Windows prev app handle
{
    return 0;
}

 int qWinAppCmdShow()                        // get main window show command
{

    STARTUPINFO startupInfo;
    GetStartupInfo(&startupInfo);

    return (startupInfo.dwFlags & STARTF_USESHOWWINDOW)
        ? startupInfo.wShowWindow
        : SW_SHOWDEFAULT;

}

void QCoreApplicationPrivate::removePostedTimerEvent(QObject *object, int timerId)
{
    QThreadData *data = object->d_func()->threadData;

    QMutexLocker locker(&data->postEventList.mutex);
    if (data->postEventList.size() == 0)
        return;
    for (int i = 0; i < data->postEventList.size(); ++i) {
        const QPostEvent &pe = data->postEventList.at(i);
        if (pe.receiver == object
                && pe.event
                && (pe.event->type() == QEvent::Timer || pe.event->type() == QEvent::ZeroTimerEvent)
                && static_cast<QTimerEvent *>(pe.event)->timerId() == timerId) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            delete pe.event;
            const_cast<QPostEvent &>(pe).event = 0;
            return;
        }
    }
}


QT_END_NAMESPACE
