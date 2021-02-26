#include "qmargins.h"
#include "qdatastream.h"

#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE


QDataStream &operator<<(QDataStream &s, const QMargins &m)
{
    s << m.left() << m.top() << m.right() << m.bottom();
    return s;
}


QDataStream &operator>>(QDataStream &s, QMargins &m)
{
    int left, top, right, bottom;
    s >> left; m.setLeft(left);
    s >> top; m.setTop(top);
    s >> right; m.setRight(right);
    s >> bottom; m.setBottom(bottom);
    return s;
}


QDebug operator<<(QDebug dbg, const QMargins &m)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QMargins" << '(';
    QtDebugUtils::formatQMargins(dbg, m);
    dbg << ')';
    return dbg;
}

QDataStream &operator<<(QDataStream &s, const QMarginsF &m)
{
    s << double(m.left()) << double(m.top()) << double(m.right()) << double(m.bottom());
    return s;
}

QDataStream &operator>>(QDataStream &s, QMarginsF &m)
{
    double left, top, right, bottom;
    s >> left;
    s >> top;
    s >> right;
    s >> bottom;
    m = QMarginsF(qreal(left), qreal(top), qreal(right), qreal(bottom));
    return s;
}

QDebug operator<<(QDebug dbg, const QMarginsF &m)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QMarginsF" << '(';
    QtDebugUtils::formatQMargins(dbg, m);
    dbg << ')';
    return dbg;
}

QT_END_NAMESPACE
