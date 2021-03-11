#ifndef QLABEL_H
#define QLABEL_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qframe.h>

QT_REQUIRE_CONFIG(label);

class QLabelPrivate;
class  QLabel : public QFrame
{
    //Q_OBJECT
    //Q_PROPERTY(QString text READ text WRITE setText)
//    Q_PROPERTY(Qt::TextFormat textFormat READ textFormat WRITE setTextFormat)
//    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
//    Q_PROPERTY(bool scaledContents READ hasScaledContents WRITE setScaledContents)
//    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
//    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
//    Q_PROPERTY(int margin READ margin WRITE setMargin)
//    Q_PROPERTY(int indent READ indent WRITE setIndent)
//    Q_PROPERTY(bool openExternalLinks READ openExternalLinks WRITE setOpenExternalLinks)
//    Q_PROPERTY(Qt::TextInteractionFlags textInteractionFlags READ textInteractionFlags WRITE setTextInteractionFlags)
//    Q_PROPERTY(bool hasSelectedText READ hasSelectedText)
//    Q_PROPERTY(QString selectedText READ selectedText)

public:
    explicit QLabel(QWidget *parent=Q_NULLPTR, Qt::WindowFlags f=Qt::WindowFlags());
    explicit QLabel(const QString &text, QWidget *parent=Q_NULLPTR, Qt::WindowFlags f=Qt::WindowFlags());
    ~QLabel();

public Q_SLOTS:
    void setText(const QString &);
    void setPixmap(const QPixmap &);
    void setPicture(const QPicture &);
    void setMovie(QMovie *movie);
    void setNum(int);
    void setNum(double);
    void clear();

Q_SIGNALS:
    void linkActivated(const QString& link);
    void linkHovered(const QString& link);
public:
    QString text() const;
    const QPixmap *pixmap() const;
    const QPicture *picture() const;
    QMovie *movie() const;
public:
    Qt::TextFormat textFormat() const;
    void setTextFormat(Qt::TextFormat);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment);

    void setWordWrap(bool on);
    bool wordWrap() const;

    int indent() const;
    void setIndent(int);

    int margin() const;
    void setMargin(int);

    bool hasScaledContents() const;
    void setScaledContents(bool);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
#ifndef QT_NO_SHORTCUT
    void setBuddy(QWidget *);
    QWidget *buddy() const;
#endif
    int heightForWidth(int) const override;

    bool openExternalLinks() const;
    void setOpenExternalLinks(bool open);

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    void setSelection(int, int);
    bool hasSelectedText() const;
    QString selectedText() const;
    int selectionStart() const;

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void paintEvent(QPaintEvent *) override;
    void changeEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *ev) override;
#endif // QT_NO_CONTEXTMENU
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    bool focusNextPrevChild(bool next) override;


private:
   // Q_DISABLE_COPY(QLabel)
    //Q_DECLARE_PRIVATE(QLabel)
    //Q_PRIVATE_SLOT(d_func(), void _q_movieUpdated(const QRect&))
    //Q_PRIVATE_SLOT(d_func(), void _q_movieResized(const QSize&))
    //Q_PRIVATE_SLOT(d_func(), void _q_linkHovered(const QString &))

    friend class QTipLabel;
    friend class QMessageBoxPrivate;
    friend class QBalloonTip;
};


#endif // QLABEL_H
