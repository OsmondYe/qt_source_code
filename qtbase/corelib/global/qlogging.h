#include <QtCore/qglobal.h>

#ifndef QLOGGING_H
#define QLOGGING_H



QT_BEGIN_NAMESPACE



enum QtMsgType { QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtInfoMsg, QtSystemMsg = QtCriticalMsg };

class QMessageLogContext
{
    Q_DISABLE_COPY(QMessageLogContext)
public:
    Q_DECL_CONSTEXPR QMessageLogContext()
        : version(2), line(0), file(Q_NULLPTR), function(Q_NULLPTR), category(Q_NULLPTR) {}
    Q_DECL_CONSTEXPR QMessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
        : version(2), line(lineNumber), file(fileName), function(functionName), category(categoryName) {}

    void copy(const QMessageLogContext &logContext);

    int version;
    int line;
    const char *file;
    const char *function;
    const char *category;

private:
    friend class QMessageLogger;
    friend class QDebug;
};

class QLoggingCategory;

class Q_CORE_EXPORT QMessageLogger
{
    Q_DISABLE_COPY(QMessageLogger)
public:
    Q_DECL_CONSTEXPR QMessageLogger() : context() {}
    Q_DECL_CONSTEXPR QMessageLogger(const char *file, int line, const char *function)
        : context(file, line, function, "default") {}
    Q_DECL_CONSTEXPR QMessageLogger(const char *file, int line, const char *function, const char *category)
        : context(file, line, function, category) {}

    void debug(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void noDebug(const char *, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3)
    {}
    void info(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void warning(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void critical(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

    typedef const QLoggingCategory &(*CategoryFunction)();

    void debug(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void debug(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void info(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void info(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void warning(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void warning(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void critical(const QLoggingCategory &cat, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);
    void critical(CategoryFunction catFunc, const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);

#ifndef Q_CC_MSVC
    Q_NORETURN
#endif
    void fatal(const char *msg, ...) const Q_DECL_NOTHROW Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

#ifndef QT_NO_DEBUG_STREAM
    QDebug debug() const;
    QDebug debug(const QLoggingCategory &cat) const;
    QDebug debug(CategoryFunction catFunc) const;
    QDebug info() const;
    QDebug info(const QLoggingCategory &cat) const;
    QDebug info(CategoryFunction catFunc) const;
    QDebug warning() const;
    QDebug warning(const QLoggingCategory &cat) const;
    QDebug warning(CategoryFunction catFunc) const;
    QDebug critical() const;
    QDebug critical(const QLoggingCategory &cat) const;
    QDebug critical(CategoryFunction catFunc) const;

    QNoDebug noDebug() const Q_DECL_NOTHROW;
#endif // QT_NO_DEBUG_STREAM

private:
    QMessageLogContext context;
};

#define QT_NO_MESSAGELOGCONTEXT
#define QT_MESSAGELOGCONTEXT


#define QT_MESSAGELOG_FILE __FILE__
#define QT_MESSAGELOG_LINE __LINE__
#define QT_MESSAGELOG_FUNC Q_FUNC_INFO


#define qDebug QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug
#define qInfo QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).info
#define qWarning QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).warning
#define qCritical QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).critical
#define qFatal QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).fatal

#define QT_NO_QDEBUG_MACRO while (false) QMessageLogger().noDebug



Q_CORE_EXPORT void qt_message_output(QtMsgType, const QMessageLogContext &context,
                                     const QString &message);

Q_CORE_EXPORT void qErrnoWarning(int code, const char *msg, ...);
Q_CORE_EXPORT void qErrnoWarning(const char *msg, ...);

#if QT_DEPRECATED_SINCE(5, 0)// deprecated. Use qInstallMessageHandler instead!
typedef void (*QtMsgHandler)(QtMsgType, const char *);
Q_CORE_EXPORT QT_DEPRECATED QtMsgHandler qInstallMsgHandler(QtMsgHandler);
#endif

typedef void (*QtMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
Q_CORE_EXPORT QtMessageHandler qInstallMessageHandler(QtMessageHandler);

Q_CORE_EXPORT void qSetMessagePattern(const QString &messagePattern);
Q_CORE_EXPORT QString qFormatLogMessage(QtMsgType type, const QMessageLogContext &context,
                                        const QString &buf);

QT_END_NAMESPACE
#endif // QLOGGING_H
