#ifndef QWINEVENTNOTIFIER_H
#define QWINEVENTNOTIFIER_H

#include "QtCore/qobject.h"


QT_BEGIN_NAMESPACE

class QWinEventNotifierPrivate;
class QWinEventNotifier : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWinEventNotifier)
    typedef Qt::HANDLE HANDLE;

public:
    explicit QWinEventNotifier(QObject *parent = Q_NULLPTR);
    explicit QWinEventNotifier(HANDLE hEvent, QObject *parent = Q_NULLPTR);
    ~QWinEventNotifier();

    void setHandle(HANDLE hEvent);
    HANDLE handle() const;

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enable);

Q_SIGNALS:
    void activated(HANDLE hEvent, QPrivateSignal);

protected:
    bool event(QEvent * e);
};

QT_END_NAMESPACE



#endif // QWINEVENTNOTIFIER_H
