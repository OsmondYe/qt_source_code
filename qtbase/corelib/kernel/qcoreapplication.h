#ifndef QCOREAPPLICATION_H
#define QCOREAPPLICATION_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#include <QtCore/qobject.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qeventloop.h>

#include <QtCore/qscopedpointer.h>


typedef struct tagMSG MSG;


class QCoreApplicationPrivate;
class QTextCodec;
class QTranslator;
class QPostEventList;
class QStringList;
class QAbstractEventDispatcher;
class QAbstractNativeEventFilter;

#define qApp QCoreApplication::instance()

/*
oye
	postEvent ->Native win32Dispatcher->QCoreApplicationPrivate::sendPostedEvents -> QCoreApplication::sendEvent
	// postEvent中间转了一圈,又回到了sendEvent	
	sendEvent  ------------->            
	sendSpontaneousEvent ---> notifyInternal2 
						
						-> virutal notfiy{
							// 事件过滤
							-> sendThroughApplicationEventFilters
							-> sendThroughObjectEventFilters
							#1 qApp -> handle -> widget obj
							#2 qGui -> handle -> windows obj
							#3 qCore-> handle -> other obj	
							// 三个一直的派生类,处理思路是相似的,先做过滤
						}  
						--->receiver->event(){
							# QObject里面的定义的虚函数event,再次产生虚机制
							# widget的子类率先处理, 让后事件依次向上传递, 直到基类
						}
*/
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
public Q_SLOTS:
    static void quit();

Q_SIGNALS:
    void aboutToQuit(QPrivateSignal);

    void organizationNameChanged();
    void organizationDomainChanged();
    void applicationNameChanged();
    void applicationVersionChanged();

public:
	void installNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    void removeNativeEventFilter(QAbstractNativeEventFilter *filterObj);
public:  // oye 比较重要的

	// oye, send Qt defined event
	static bool sendEvent(QObject *receiver, QEvent *event){  
		if (event) 	event->spont = false; 
		return notifyInternal2(receiver, event); 
	}	
	static void postEvent(QObject *receiver, QEvent *event, int priority = Qt::NormalEventPriority);
	static int exec();

	// GUI的App类会重写此函数
	virtual bool notify(QObject *, QEvent *)override;
    static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    static void processEvents(QEventLoop::ProcessEventsFlags flags, int maxtime);
	
	static void sendPostedEvents(QObject *receiver = Q_NULLPTR, int event_type = 0);
	static void removePostedEvents(QObject *receiver, int eventType = 0);
	// Dispatcher
	static QAbstractEventDispatcher *eventDispatcher();
	static void setEventDispatcher(QAbstractEventDispatcher *eventDispatcher);

private:

	//
	//	oye, 我看到 Pushbutton的paintEvent会来自这里, 
	//		谁call的他? 
	//
	inline bool static sendSpontaneousEvent(QObject *receiver, QEvent *event)
	{ 
		if (event) event->spont = true; 
		return notifyInternal2(receiver, event); 
	}

	// oye 这里面会形成一个dispatch call -> notify()
	// 因为notify是一个虚函数, 所以在GUI情况下,有QApplication来承载
	static bool notifyInternal2(QObject *receiver, QEvent *);
public:
	static void exit(int retcode=0);
    static bool startingUp();
    static bool closingDown();
    static bool isQuitLockEnabled();
    static void setQuitLockEnabled(bool enabled);

public:	
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
protected:
    bool event(QEvent *) ;
    virtual bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);
protected:
    QCoreApplication(QCoreApplicationPrivate &p);   
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
    friend QString qAppName();
    friend class QClassFactory;
};

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

#define Q_COREAPP_STARTUP_FUNCTION(AFUNC) 

#endif // QCOREAPPLICATION_H
