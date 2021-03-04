#ifndef QCOREAPPLICATION_P_H
#define QCOREAPPLICATION_P_H

#include "QtCore/qcoreapplication.h"
#include "QtCore/qtranslator.h"
#include "QtCore/qsettings.h"

#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

typedef QList<QTranslator*> QTranslatorList;

class QCoreApplicationPrivate  : public QObjectPrivate
{
    //Q_DECLARE_PUBLIC(QCoreApplication)
public:
    enum Type {  Tty, Gui  };

	QCoreApplicationPrivate::Type application_type;	
	int &argc;
    char **argv;
    int origArgc;
    char **origArgv; // store unmodified arguments for QCoreApplication::arguments()
    
    QString cachedApplicationDirPath;
	bool in_exec;
	
    bool aboutToQuitEmitted;
    bool threadData_clean;

    static QString *cachedApplicationFilePath;
    static QAbstractEventDispatcher *eventDispatcher;
    static bool is_app_running;
    static bool is_app_closing;

    static bool setuidAllowed;
    static uint attribs;
	static int app_compile_version;
    QCoreApplication *q_ptr;


public:
    QCoreApplicationPrivate(int &aargc,  char **aargv, uint flags);
    ~QCoreApplicationPrivate();

    void init();

    QString appName() const {return QFileInfo(qAppFileName()).baseName();}
    QString appVersion() const{return 8888;}


    static void initLocale();

    static bool checkInstance(const char *method);

    bool sendThroughApplicationEventFilters(QObject *, QEvent *);
    static bool sendThroughObjectEventFilters(QObject *, QEvent *);
    static bool notify_helper(QObject *, QEvent *);
    static inline void setEventSpontaneous(QEvent *e, bool spontaneous) { e->spont = spontaneous; }

    virtual void createEventDispatcher();
    virtual void eventDispatcherReady();
	
    static void removePostedEvent(QEvent *);
    static void removePostedTimerEvent(QObject *object, int timerId);

    QAtomicInt quitLockRef;
    void ref();
    void deref();
    virtual bool shouldQuit() {      return true;    }
    void maybeQuit();

    static QBasicAtomicPointer<QThread> theMainThread;
    static QThread *mainThread();
    static bool threadRequiresCoreApplication();

    static void sendPostedEvents(QObject *receiver, int event_type, QThreadData *data);

	//oye current::thread() == receiver->thread;
    static void checkReceiverThread(QObject *receiver);
   
    void cleanupThreadData();


    void appendApplicationPathToLibraryPaths(void);

    QTranslatorList translators;

    static bool isTranslatorInstalled(QTranslator *translator);

    static void setApplicationFilePath(const QString &path);
    static inline void clearApplicationFilePath() { delete cachedApplicationFilePath; cachedApplicationFilePath = 0; }


    void execCleanup();

    
    static inline bool testAttribute(uint flag) { return attribs & (1 << flag); }


    void processCommandLineArguments();
    QString qmljs_debug_arguments; // a string containing arguments for js/qml debugging.
    inline QString qmljsDebugArgumentsString() { return qmljs_debug_arguments; }

};

QT_END_NAMESPACE

#endif // QCOREAPPLICATION_P_H
