#ifndef QPOINT_H
#define QPOINT_H

#include <QtCore/qnamespace.h>





class  QPoint
{
public:
     QPoint();
     QPoint(int xpos, int ypos);

     inline bool isNull() const;

     inline int x() const;
     inline int y() const;
     inline void setX(int x);
     inline void setY(int y);

     inline int manhattanLength() const;

     inline int &rx();
     inline int &ry();

     inline QPoint &operator+=(const QPoint &p);
     inline QPoint &operator-=(const QPoint &p);

     inline QPoint &operator*=(float factor);
     inline QPoint &operator*=(double factor);
     inline QPoint &operator*=(int factor);

     inline QPoint &operator/=(qreal divisor);

     static inline int dotProduct(const QPoint &p1, const QPoint &p2)
    { return p1.xp * p2.xp + p1.yp * p2.yp; }

    friend  inline bool operator==(const QPoint &, const QPoint &);
    friend  inline bool operator!=(const QPoint &, const QPoint &);
    friend  inline const QPoint operator+(const QPoint &, const QPoint &);
    friend  inline const QPoint operator-(const QPoint &, const QPoint &);
    friend  inline const QPoint operator*(const QPoint &, float);
    friend  inline const QPoint operator*(float, const QPoint &);
    friend  inline const QPoint operator*(const QPoint &, double);
    friend  inline const QPoint operator*(double, const QPoint &);
    friend  inline const QPoint operator*(const QPoint &, int);
    friend  inline const QPoint operator*(int, const QPoint &);
    friend  inline const QPoint operator+(const QPoint &);
    friend  inline const QPoint operator-(const QPoint &);
    friend  inline const QPoint operator/(const QPoint &, qreal);


private:
    friend class QTransform;
    int xp;
    int yp;
};





/*****************************************************************************
  QPoint inline functions
 *****************************************************************************/

 inline QPoint::QPoint() : xp(0), yp(0) {}

 inline QPoint::QPoint(int xpos, int ypos) : xp(xpos), yp(ypos) {}

 inline bool QPoint::isNull() const
{ return xp == 0 && yp == 0; }

 inline int QPoint::x() const
{ return xp; }

 inline int QPoint::y() const
{ return yp; }

 inline void QPoint::setX(int xpos)
{ xp = xpos; }

 inline void QPoint::setY(int ypos)
{ yp = ypos; }

inline int  QPoint::manhattanLength() const
{ return qAbs(x())+qAbs(y()); }

 inline int &QPoint::rx()
{ return xp; }

 inline int &QPoint::ry()
{ return yp; }

 inline QPoint &QPoint::operator+=(const QPoint &p)
{ xp+=p.xp; yp+=p.yp; return *this; }

 inline QPoint &QPoint::operator-=(const QPoint &p)
{ xp-=p.xp; yp-=p.yp; return *this; }

 inline QPoint &QPoint::operator*=(float factor)
{ xp = qRound(xp*factor); yp = qRound(yp*factor); return *this; }

 inline QPoint &QPoint::operator*=(double factor)
{ xp = qRound(xp*factor); yp = qRound(yp*factor); return *this; }

 inline QPoint &QPoint::operator*=(int factor)
{ xp = xp*factor; yp = yp*factor; return *this; }

 inline bool operator==(const QPoint &p1, const QPoint &p2)
{ return p1.xp == p2.xp && p1.yp == p2.yp; }

 inline bool operator!=(const QPoint &p1, const QPoint &p2)
{ return p1.xp != p2.xp || p1.yp != p2.yp; }

 inline const QPoint operator+(const QPoint &p1, const QPoint &p2)
{ return QPoint(p1.xp+p2.xp, p1.yp+p2.yp); }

 inline const QPoint operator-(const QPoint &p1, const QPoint &p2)
{ return QPoint(p1.xp-p2.xp, p1.yp-p2.yp); }

 inline const QPoint operator*(const QPoint &p, float factor)
{ return QPoint(qRound(p.xp*factor), qRound(p.yp*factor)); }

 inline const QPoint operator*(const QPoint &p, double factor)
{ return QPoint(qRound(p.xp*factor), qRound(p.yp*factor)); }

 inline const QPoint operator*(const QPoint &p, int factor)
{ return QPoint(p.xp*factor, p.yp*factor); }

 inline const QPoint operator*(float factor, const QPoint &p)
{ return QPoint(qRound(p.xp*factor), qRound(p.yp*factor)); }

 inline const QPoint operator*(double factor, const QPoint &p)
{ return QPoint(qRound(p.xp*factor), qRound(p.yp*factor)); }

 inline const QPoint operator*(int factor, const QPoint &p)
{ return QPoint(p.xp*factor, p.yp*factor); }

 inline const QPoint operator+(const QPoint &p)
{ return p; }

 inline const QPoint operator-(const QPoint &p)
{ return QPoint(-p.xp, -p.yp); }

 inline QPoint &QPoint::operator/=(qreal c)
{
    xp = qRound(xp/c);
    yp = qRound(yp/c);
    return *this;
}

 inline const QPoint operator/(const QPoint &p, qreal c)
{
    return QPoint(qRound(p.xp/c), qRound(p.yp/c));
}







class  QPointF
{
public:
     QPointF();
     QPointF(const QPoint &p);
     QPointF(qreal xpos, qreal ypos);

     inline qreal manhattanLength() const;

    inline bool isNull() const;

     inline qreal x() const;
     inline qreal y() const;
     inline void setX(qreal x);
     inline void setY(qreal y);

     inline qreal &rx();
     inline qreal &ry();

     inline QPointF &operator+=(const QPointF &p);
     inline QPointF &operator-=(const QPointF &p);
     inline QPointF &operator*=(qreal c);
     inline QPointF &operator/=(qreal c);

     static inline qreal dotProduct(const QPointF &p1, const QPointF &p2)
    { return p1.xp * p2.xp + p1.yp * p2.yp; }

    friend  inline bool operator==(const QPointF &, const QPointF &);
    friend  inline bool operator!=(const QPointF &, const QPointF &);
    friend  inline const QPointF operator+(const QPointF &, const QPointF &);
    friend  inline const QPointF operator-(const QPointF &, const QPointF &);
    friend  inline const QPointF operator*(qreal, const QPointF &);
    friend  inline const QPointF operator*(const QPointF &, qreal);
    friend  inline const QPointF operator+(const QPointF &);
    friend  inline const QPointF operator-(const QPointF &);
    friend  inline const QPointF operator/(const QPointF &, qreal);

     QPoint toPoint() const;



private:
    friend class QMatrix;
    friend class QTransform;

    qreal xp;
    qreal yp;
};




/*****************************************************************************
  QPointF inline functions
 *****************************************************************************/

 inline QPointF::QPointF() : xp(0), yp(0) { }

 inline QPointF::QPointF(qreal xpos, qreal ypos) : xp(xpos), yp(ypos) { }

 inline QPointF::QPointF(const QPoint &p) : xp(p.x()), yp(p.y()) { }

 inline qreal QPointF::manhattanLength() const
{
    return qAbs(x())+qAbs(y());
}

inline bool QPointF::isNull() const
{
    return qIsNull(xp) && qIsNull(yp);
}

 inline qreal QPointF::x() const
{
    return xp;
}

 inline qreal QPointF::y() const
{
    return yp;
}

 inline void QPointF::setX(qreal xpos)
{
    xp = xpos;
}

 inline void QPointF::setY(qreal ypos)
{
    yp = ypos;
}

 inline qreal &QPointF::rx()
{
    return xp;
}

 inline qreal &QPointF::ry()
{
    return yp;
}

 inline QPointF &QPointF::operator+=(const QPointF &p)
{
    xp+=p.xp;
    yp+=p.yp;
    return *this;
}

 inline QPointF &QPointF::operator-=(const QPointF &p)
{
    xp-=p.xp; yp-=p.yp; return *this;
}

 inline QPointF &QPointF::operator*=(qreal c)
{
    xp*=c; yp*=c; return *this;
}

 inline bool operator==(const QPointF &p1, const QPointF &p2)
{
    return qFuzzyIsNull(p1.xp - p2.xp) && qFuzzyIsNull(p1.yp - p2.yp);
}

 inline bool operator!=(const QPointF &p1, const QPointF &p2)
{
    return !qFuzzyIsNull(p1.xp - p2.xp) || !qFuzzyIsNull(p1.yp - p2.yp);
}

 inline const QPointF operator+(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.xp+p2.xp, p1.yp+p2.yp);
}

 inline const QPointF operator-(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.xp-p2.xp, p1.yp-p2.yp);
}

 inline const QPointF operator*(const QPointF &p, qreal c)
{
    return QPointF(p.xp*c, p.yp*c);
}

 inline const QPointF operator*(qreal c, const QPointF &p)
{
    return QPointF(p.xp*c, p.yp*c);
}

 inline const QPointF operator+(const QPointF &p)
{
    return p;
}

 inline const QPointF operator-(const QPointF &p)
{
    return QPointF(-p.xp, -p.yp);
}

 inline QPointF &QPointF::operator/=(qreal divisor)
{
    xp/=divisor;
    yp/=divisor;
    return *this;
}

 inline const QPointF operator/(const QPointF &p, qreal divisor)
{
    return QPointF(p.xp/divisor, p.yp/divisor);
}

 inline QPoint QPointF::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}



#endif // QPOINT_H
