#ifndef QCOREAPPLICATION_H
#define QCOREAPPLICATION_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#ifndef QT_NO_QOBJECT
#include <QtCore/qobject.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qeventloop.h>
#else
#include <QtCore/qscopedpointer.h>
#endif

#ifndef QT_NO_QOBJECT
#if defined(Q_OS_WIN) && !defined(tagMSG)
typedef struct tagMSG MSG;
#endif
#endif

QT_BEGIN_NAMESPACE


class QCoreApplicationPrivate;
class QTextCodec;
class QTranslator;
class QPostEventList;
class QStringList;
class QAbstractEventDispatcher;
class QAbstractNativeEventFilter;

#define qApp QCoreApplication::instance()

class  QCoreApplication    : public QObject
{

public:
    enum { ApplicationFlags = QT_VERSION    };

    QCoreApplication(int &argc, char **argv
#ifndef Q_QDOC
                     , int = ApplicationFlags
#endif
            );

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

    static void setSetuidAllowed(bool allow);
    static bool isSetuidAllowed();

    static QCoreApplication *instance() { return self; }

#ifndef QT_NO_QOBJECT
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
#endif

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
#if QT_DEPRECATED_SINCE(5, 0)
    enum Encoding { UnicodeUTF8, Latin1, DefaultCodec = UnicodeUTF8, CodecForTr = UnicodeUTF8 };
    QT_DEPRECATED static inline QString translate(const char * context, const char * key,
                             const char * disambiguation, Encoding, int n = -1)
        { return translate(context, key, disambiguation, n); }
#endif


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
    bool event(QEvent *) Q_DECL_OVERRIDE;

    virtual bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);

protected:
    QCoreApplication(QCoreApplicationPrivate &p);

    QScopedPointer<QCoreApplicationPrivate> d_ptr;

private:
    static bool sendSpontaneousEvent(QObject *receiver, QEvent *event);
    static bool notifyInternal2(QObject *receiver, QEvent *);

    static QCoreApplication *self;

    Q_DISABLE_COPY(QCoreApplication)

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

#ifdef QT_NO_DEPRECATED
#  define QT_DECLARE_DEPRECATED_TR_FUNCTIONS(context)
#else
#  define QT_DECLARE_DEPRECATED_TR_FUNCTIONS(context) \
    QT_DEPRECATED static inline QString trUtf8(const char *sourceText, const char *disambiguation = Q_NULLPTR, int n = -1) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, n); }
#endif

#define Q_DECLARE_TR_FUNCTIONS(context) \
public: \
    static inline QString tr(const char *sourceText, const char *disambiguation = Q_NULLPTR, int n = -1) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, n); } \
    QT_DECLARE_DEPRECATED_TR_FUNCTIONS(context) \
private:

typedef void (*QtStartUpFunction)();
typedef void (*QtCleanUpFunction)();

Q_CORE_EXPORT void qAddPreRoutine(QtStartUpFunction);
Q_CORE_EXPORT void qAddPostRoutine(QtCleanUpFunction);
Q_CORE_EXPORT void qRemovePostRoutine(QtCleanUpFunction);
Q_CORE_EXPORT QString qAppName();                // get application name

#define Q_COREAPP_STARTUP_FUNCTION(AFUNC) \
    static void AFUNC ## _ctor_function() {  \
        qAddPreRoutine(AFUNC);        \
    }                                 \
    Q_CONSTRUCTOR_FUNCTION(AFUNC ## _ctor_function)


QT_END_NAMESPACE

#endif // QCOREAPPLICATION_H
