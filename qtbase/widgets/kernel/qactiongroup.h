#ifndef QACTIONGROUP_H
#define QACTIONGROUP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qaction.h>

QT_BEGIN_NAMESPACE


class QActionGroupPrivate;

class  QActionGroup : public QObject
{
//    Q_OBJECT
//    Q_DECLARE_PRIVATE(QActionGroup)
//
//    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive)
//    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
//    Q_PROPERTY(bool visible READ isVisible WRITE setVisible)

public:
    explicit QActionGroup(QObject* parent);
    ~QActionGroup();

    QAction *addAction(QAction* a);
    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    void removeAction(QAction *a);
    QList<QAction*> actions() const;

    QAction *checkedAction() const;
    bool isExclusive() const;
    bool isEnabled() const;
    bool isVisible() const;


public Q_SLOTS:
    void setEnabled(bool);
    inline void setDisabled(bool b) { setEnabled(!b); }
    void setVisible(bool);
    void setExclusive(bool);

Q_SIGNALS:
    void triggered(QAction *);
    void hovered(QAction *);

private:
    Q_DISABLE_COPY(QActionGroup)
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())
    Q_PRIVATE_SLOT(d_func(), void _q_actionChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_actionHovered())
};

QT_END_NAMESPACE

#endif // QACTIONGROUP_H
