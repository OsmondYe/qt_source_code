#ifndef QCOREAPPLICATION_H
#define QCOREAPPLICATION_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#include <QtCore/qobject.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qeventloop.h>

#include <QtCore/qscopedpointer.h>


typedef struct tagMSG MSG;

QT_BEGIN_NAMESPACE

class QCoreApplicationPrivate;
class QTextCodec;
class QTranslator;
class QPostEventList;
class QStringList;
class QAbstractEventDispatcher;
class QAbstractNativeEventFilter;

#define qApp QCoreApplication::instance()

// Singlton
class  QCoreApplication    : public QObject
{

protected:
	QScopedPointer<QCoreApplicationPrivate> d_ptr; // oye, delegation to PrivateClass to impl
private:
	static QCoreApplication *self;  // oye, Singleton

public:
    static QCoreApplication *instance() { return self; }
    enum { ApplicationFlags = QT_VERSION };

    QCoreApplication(int &argc, char **argv      , int = ApplicationFlags );

    ~QCoreApplication();

    static QStringList arguments();

    static void setAttribute(Qt::ApplicationAttribute attribute, bool on = true);
    static bool testAttribute(Qt::ApplicationAttribute attribute);

    static void setOrganizationDomain(const QString &orgDomain);
    static QString organizationDomain();
    static void setOrganizationName(const QString &orgName);
    static QString organizationName();
    static void setApplicationName(const QString &application);
    static QString applicationName();
    static void setApplicationVersion(const QString &version);
    static QString applicationVersion();


    static int exec();
    static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    static void processEvents(QEventLoop::ProcessEventsFlags flags, int maxtime);
    static void exit(int retcode=0);

	// oye:  old friend
    static bool sendEvent(QObject *receiver, QEvent *event);
    static void postEvent(QObject *receiver, QEvent *event, int priority = Qt::NormalEventPriority);
	
    static void sendPostedEvents(QObject *receiver = Q_NULLPTR, int event_type = 0);
    static void removePostedEvents(QObject *receiver, int eventType = 0);

    static QAbstractEventDispatcher *eventDispatcher();
    static void setEventDispatcher(QAbstractEventDispatcher *eventDispatcher);

    virtual bool notify(QObject *, QEvent *);
	

    static bool startingUp();
    static bool closingDown();


    static QString applicationDirPath();
    static QString applicationFilePath();
    static qint64 applicationPid();

#if QT_CONFIG(library)
    static void setLibraryPaths(const QStringList &);
    static QStringList libraryPaths();
    static void addLibraryPath(const QString &);
    static void removeLibraryPath(const QString &);
#endif // QT_CONFIG(library)

#ifndef QT_NO_TRANSLATION
    static bool installTranslator(QTranslator * messageFile);
    static bool removeTranslator(QTranslator * messageFile);
#endif

    static QString translate(const char * context,
                             const char * key,
                             const char * disambiguation = Q_NULLPTR,
                             int n = -1);


    void installNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    void removeNativeEventFilter(QAbstractNativeEventFilter *filterObj);

    static bool isQuitLockEnabled();
    static void setQuitLockEnabled(bool enabled);

public Q_SLOTS:
    static void quit();

Q_SIGNALS:
    void aboutToQuit(QPrivateSignal);

    void organizationNameChanged();
    void organizationDomainChanged();
    void applicationNameChanged();
    void applicationVersionChanged();

protected:
    bool event(QEvent *) ;

    virtual bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);

protected:
    QCoreApplication(QCoreApplicationPrivate &p);   

private:
    static bool sendSpontaneousEvent(QObject *receiver, QEvent *event);
    static bool notifyInternal2(QObject *receiver, QEvent *);

   // Q_DISABLE_COPY(QCoreApplication)

    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QWidget;
    friend class QWidgetWindow;
    friend class QWidgetPrivate;
    friend class QEventDispatcherUNIXPrivate;
    friend class QCocoaEventDispatcherPrivate;
    friend bool qt_sendSpontaneousEvent(QObject*, QEvent*);
    friend Q_CORE_EXPORT QString qAppName();
    friend class QClassFactory;
};

inline bool QCoreApplication::sendEvent(QObject *receiver, QEvent *event)
{  if (event) event->spont = false; return notifyInternal2(receiver, event); }

inline bool QCoreApplication::sendSpontaneousEvent(QObject *receiver, QEvent *event)
{ if (event) event->spont = true; return notifyInternal2(receiver, event); }


#define QT_DECLARE_DEPRECATED_TR_FUNCTIONS(context)


#define Q_DECLARE_TR_FUNCTIONS(context) \
public: \
    static inline QString tr(const char *sourceText, const char *disambiguation = Q_NULLPTR, int n = -1) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, n); } \
    QT_DECLARE_DEPRECATED_TR_FUNCTIONS(context) \
private:

typedef void (*QtStartUpFunction)();
typedef void (*QtCleanUpFunction)();

 void qAddPreRoutine(QtStartUpFunction);
 void qAddPostRoutine(QtCleanUpFunction);
 void qRemovePostRoutine(QtCleanUpFunction);
 QString qAppName();                // get application name

#define Q_COREAPP_STARTUP_FUNCTION(AFUNC) \
    static void AFUNC ## _ctor_function() {  \
        qAddPreRoutine(AFUNC);        \
    }                                 \
    Q_CONSTRUCTOR_FUNCTION(AFUNC ## _ctor_function)


QT_END_NAMESPACE

#endif // QCOREAPPLICATION_H
