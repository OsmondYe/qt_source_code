#ifndef QCOREAPPLICATION_P_H
#define QCOREAPPLICATION_P_H

#include "QtCore/qcoreapplication.h"
#include "QtCore/qtranslator.h"
#include "QtCore/qsettings.h"

#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

typedef QList<QTranslator*> QTranslatorList;

class QAbstractEventDispatcher;

class QCoreApplicationPrivate     : public QObjectPrivate

{
    Q_DECLARE_PUBLIC(QCoreApplication)

public:
    enum Type {
        Tty,
        Gui
    };

    QCoreApplicationPrivate(int &aargc,  char **aargv, uint flags);
    ~QCoreApplicationPrivate();

    void init();

    QString appName() const;
    QString appVersion() const;


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
    virtual bool shouldQuit() {
      return true;
    }
    void maybeQuit();

    static QBasicAtomicPointer<QThread> theMainThread;
    static QThread *mainThread();
    static bool threadRequiresCoreApplication();

    static void sendPostedEvents(QObject *receiver, int event_type, QThreadData *data);

    static void checkReceiverThread(QObject *receiver);
    void cleanupThreadData();


    int &argc;
    char **argv;

    int origArgc;
    char **origArgv; // store unmodified arguments for QCoreApplication::arguments()

    void appendApplicationPathToLibraryPaths(void);


    QTranslatorList translators;

    static bool isTranslatorInstalled(QTranslator *translator);


    QCoreApplicationPrivate::Type application_type;

    QString cachedApplicationDirPath;
    static QString *cachedApplicationFilePath;
    static void setApplicationFilePath(const QString &path);
    static inline void clearApplicationFilePath() { delete cachedApplicationFilePath; cachedApplicationFilePath = 0; }


    void execCleanup();

    bool in_exec;
    bool aboutToQuitEmitted;
    bool threadData_clean;

    static QAbstractEventDispatcher *eventDispatcher;
    static bool is_app_running;
    static bool is_app_closing;


    static bool setuidAllowed;
    static uint attribs;
    static inline bool testAttribute(uint flag) { return attribs & (1 << flag); }
    static int app_compile_version;

    void processCommandLineArguments();
    QString qmljs_debug_arguments; // a string containing arguments for js/qml debugging.
    inline QString qmljsDebugArgumentsString() { return qmljs_debug_arguments; }


    QCoreApplication *q_ptr;

};

QT_END_NAMESPACE

#endif // QCOREAPPLICATION_P_H
