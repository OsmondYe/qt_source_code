#ifndef QPUSHBUTTON_H
#define QPUSHBUTTON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractbutton.h>

QT_REQUIRE_CONFIG(pushbutton);

QT_BEGIN_NAMESPACE


class QPushButtonPrivate;
class QMenu;
class QStyleOptionButton;

class  QPushButton : public QAbstractButton
{
    //Q_OBJECT

    //Q_PROPERTY(bool autoDefault READ autoDefault WRITE setAutoDefault)
    //Q_PROPERTY(bool default READ isDefault WRITE setDefault)
    //Q_PROPERTY(bool flat READ isFlat WRITE setFlat)

public:
    explicit QPushButton(QWidget *parent = Q_NULLPTR);
    explicit QPushButton(const QString &text, QWidget *parent = Q_NULLPTR);
    QPushButton(const QIcon& icon, const QString &text, QWidget *parent = Q_NULLPTR);
    ~QPushButton();

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    bool autoDefault() const;
    void setAutoDefault(bool);
    bool isDefault() const;
    void setDefault(bool);

    void setMenu(QMenu* menu);
    QMenu* menu() const;

    void setFlat(bool);
    bool isFlat() const;

public Q_SLOTS:
    void showMenu();

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *) Q_DECL_OVERRIDE;
    void initStyleOption(QStyleOptionButton *option) const;
    QPushButton(QPushButtonPrivate &dd, QWidget* parent = Q_NULLPTR);

public:

private:
    //Q_DISABLE_COPY(QPushButton)
    //Q_DECLARE_PRIVATE(QPushButton)
    //Q_PRIVATE_SLOT(d_func(), void _q_popupPressed())
};

QT_END_NAMESPACE

#endif // QPUSHBUTTON_H
