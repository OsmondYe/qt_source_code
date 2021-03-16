#ifndef QINPUTCONTROL_P_H
#define QINPUTCONTROL_P_H


#include <QtCore/qobject.h>
#include <qtguiglobal.h>


class QKeyEvent;
class  QInputControl : public QObject
{
    //Q_OBJECT
public:
    enum Type {
        LineEdit,
        TextEdit
    };

    explicit QInputControl(Type type, QObject *parent = nullptr);

    bool isAcceptableInput(const QKeyEvent *event) const;
    static bool isCommonTextEditShortcut(const QKeyEvent *ke);

protected:
    explicit QInputControl(Type type, QObjectPrivate &dd, QObject *parent = nullptr);

private:
    const Type m_type;
};

#endif // QINPUTCONTROL_P_H
