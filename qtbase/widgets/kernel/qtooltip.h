#ifndef QTOOLTIP_H
#define QTOOLTIP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

// oye ¸ø¿Õtext,×Ô¶¯Òþ²Ø
class QToolTip
{
public:
    static void showText(const QPoint &pos, const QString &text, QWidget *w = Q_NULLPTR){
		showText(pos, text, w, QRect());
    }
    static void showText(const QPoint &pos, const QString &text, QWidget *w, const QRect &rect){
    	showText(pos, text, w, rect, -1);
    }
    static void showText(const QPoint &pos, const QString &text, QWidget *w, const QRect &rect, int msecShowTime);
    static inline void hideText() { showText(QPoint(), QString()); }

    static bool isVisible();
    static QString text();

    static QPalette palette();
    static void setPalette(const QPalette &);
    static QFont font();
    static void setFont(const QFont &);
};



#endif // QTOOLTIP_H
