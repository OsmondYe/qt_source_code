#ifndef QLABEL_P_H
#define QLABEL_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qlabel.h"

#include "private/qtextdocumentlayout_p.h"
#include "private/qwidgettextcontrol_p.h"
#include "qtextdocumentfragment.h"
#include "qframe_p.h"
#include "qtextdocument.h"
#if QT_CONFIG(movie)
#include "qmovie.h"
#endif
#include "qimage.h"
#include "qbitmap.h"
#include "qpicture.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif


class QLabelPrivate : public QFramePrivate
{
    //Q_DECLARE_PUBLIC(QLabel)
public:
	mutable QSize sh;
    mutable QSize msh;
    QString text;
    QPixmap  *pixmap;
    QPixmap *scaledpixmap;
    QImage *cachedimage;
#ifndef QT_NO_PICTURE
    QPicture *picture;
#endif
#if QT_CONFIG(movie)
    QPointer<QMovie> movie;
#endif
    mutable QWidgetTextControl *control;
    mutable QTextCursor shortcutCursor;
#ifndef QT_NO_CURSOR
    QCursor cursor;
#endif
#ifndef QT_NO_SHORTCUT
    QPointer<QWidget> buddy;
    int shortcutId;
#endif
    Qt::TextFormat textformat;
    Qt::TextInteractionFlags textInteractionFlags;
    mutable QSizePolicy sizePolicy;
    int margin;
    ushort align;
    short indent;
    mutable uint valid_hints : 1;
    uint scaledcontents : 1;
    mutable uint textLayoutDirty : 1;
    mutable uint textDirty : 1;
    mutable uint isRichText : 1;
    mutable uint isTextLabel : 1;
    mutable uint hasShortcut : 1;
#ifndef QT_NO_CURSOR
    uint validCursor : 1;
    uint onAnchor : 1;
#endif
    uint openExternalLinks : 1;

public:
    QLabelPrivate();
    ~QLabelPrivate(){}

    void init();
    void clearContents();
    void updateLabel();
    QSize sizeForWidth(int w) const;

#if QT_CONFIG(movie)
    void _q_movieUpdated(const QRect&);
    void _q_movieResized(const QSize&);
#endif
#ifndef QT_NO_SHORTCUT
    void updateShortcut();
#endif
    inline bool needTextControl() const {
        return isTextLabel
               && (isRichText
                   || (!isRichText && (textInteractionFlags & (Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard))));
    }

    void ensureTextPopulated() const;
    void ensureTextLayouted() const;
    void ensureTextControl() const;
    void sendControlEvent(QEvent *e);

    void _q_linkHovered(const QString &link);

    QRectF layoutRect() const;
    QRect documentRect() const;
    QPoint layoutPoint(const QPoint& p) const;
    Qt::LayoutDirection textDirection() const;
#ifndef QT_NO_CONTEXTMENU
    QMenu *createStandardContextMenu(const QPoint &pos);
#endif


    // <-- space for more bit field values here

    friend class QMessageBoxPrivate;
};


#endif // QLABEL_P_H
