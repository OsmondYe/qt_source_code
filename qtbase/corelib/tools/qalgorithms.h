#ifndef QALGORITHMS_H
#define QALGORITHMS_H

#include <QtCore/qglobal.h>


template <typename ForwardIterator>
inline void qDeleteAll(ForwardIterator begin, ForwardIterator end)
{
    while (begin != end) {
        delete *begin;
        ++begin;
    }
}

template <typename Container>
inline void qDeleteAll(const Container &c)
{
    qDeleteAll(c.begin(), c.end());
}


namespace QAlgorithmsPrivate {

inline unsigned long qt_builtin_ctz(quint32 val)
{
    unsigned long result;
    _BitScanForward(&result, val);
    return result;
}
inline unsigned long qt_builtin_clz(quint32 val)
{
    unsigned long result;
    _BitScanReverse(&result, val);
    // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
    // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
    // starts there), and the lsb index is 31.
    result ^= sizeof(quint32) * 8 - 1;
    return result;
}
// These are only defined for 64bit builds.
inline unsigned long qt_builtin_ctzll(quint64 val)
{
    unsigned long result;
    _BitScanForward64(&result, val);
    return result;
}
inline unsigned long qt_builtin_clzll(quint64 val)
{
    unsigned long result;
    _BitScanReverse64(&result, val);
    // see qt_builtin_clz
    result ^= sizeof(quint64) * 8 - 1;
    return result;
}
inline uint qt_builtin_ctzs(quint16 v) throw()
{
    return qt_builtin_ctz(v);
}
inline uint qt_builtin_clzs(quint16 v) throw()
{
    return qt_builtin_clz(v) - 16U;
}

} //namespace QAlgorithmsPrivate

  inline uint qPopulationCount(quint32 v) throw()
{
#ifdef QALGORITHMS_USE_BUILTIN_POPCOUNT
    return QAlgorithmsPrivate::qt_builtin_popcount(v);
#else
    // See http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    return
        (((v      ) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 12) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 24) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

  inline uint qPopulationCount(quint8 v) throw()
{
#ifdef QALGORITHMS_USE_BUILTIN_POPCOUNT
    return QAlgorithmsPrivate::qt_builtin_popcount(v);
#else
    return
        (((v      ) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

  inline uint qPopulationCount(quint16 v) throw()
{
#ifdef QALGORITHMS_USE_BUILTIN_POPCOUNT
    return QAlgorithmsPrivate::qt_builtin_popcount(v);
#else
    return
        (((v      ) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 12) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

  inline uint qPopulationCount(quint64 v) throw()
{
#ifdef QALGORITHMS_USE_BUILTIN_POPCOUNTLL
    return QAlgorithmsPrivate::qt_builtin_popcountll(v);
#else
    return
        (((v      ) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 12) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 24) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 36) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 48) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
        (((v >> 60) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

  inline uint qPopulationCount(long unsigned int v) throw()
{
    return qPopulationCount(static_cast<quint64>(v));
}


 inline uint qCountTrailingZeroBits(quint32 v) throw()
{
#if defined(QT_HAS_BUILTIN_CTZ)
    return v ? QAlgorithmsPrivate::qt_builtin_ctz(v) : 32U;
#else
    // see http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel
    unsigned int c = 32; // c will be the number of zero bits on the right
    v &= -signed(v);
    if (v) c--;
    if (v & 0x0000FFFF) c -= 16;
    if (v & 0x00FF00FF) c -= 8;
    if (v & 0x0F0F0F0F) c -= 4;
    if (v & 0x33333333) c -= 2;
    if (v & 0x55555555) c -= 1;
    return c;
#endif
}

 inline uint qCountTrailingZeroBits(quint8 v) throw()
{
#if defined(QT_HAS_BUILTIN_CTZ)
    return v ? QAlgorithmsPrivate::qt_builtin_ctz(v) : 8U;
#else
    unsigned int c = 8; // c will be the number of zero bits on the right
    v &= -signed(v);
    if (v) c--;
    if (v & 0x0000000F) c -= 4;
    if (v & 0x00000033) c -= 2;
    if (v & 0x00000055) c -= 1;
    return c;
#endif
}

 inline uint qCountTrailingZeroBits(quint16 v) throw()
{
#if defined(QT_HAS_BUILTIN_CTZS)
    return v ? QAlgorithmsPrivate::qt_builtin_ctzs(v) : 16U;
#else
    unsigned int c = 16; // c will be the number of zero bits on the right
    v &= -signed(v);
    if (v) c--;
    if (v & 0x000000FF) c -= 8;
    if (v & 0x00000F0F) c -= 4;
    if (v & 0x00003333) c -= 2;
    if (v & 0x00005555) c -= 1;
    return c;
#endif
}

 inline uint qCountTrailingZeroBits(quint64 v) throw()
{
#if defined(QT_HAS_BUILTIN_CTZLL)
    return v ? QAlgorithmsPrivate::qt_builtin_ctzll(v) : 64;
#else
    quint32 x = static_cast<quint32>(v);
    return x ? qCountTrailingZeroBits(x)
             : 32 + qCountTrailingZeroBits(static_cast<quint32>(v >> 32));
#endif
}

 inline uint qCountTrailingZeroBits(unsigned long v) throw()
{
    return qCountTrailingZeroBits(QIntegerForSizeof<long>::Unsigned(v));
}

 inline uint qCountLeadingZeroBits(quint32 v) throw()
{
#if defined(QT_HAS_BUILTIN_CLZ)
    return v ? QAlgorithmsPrivate::qt_builtin_clz(v) : 32U;
#else
    // Hacker's Delight, 2nd ed. Fig 5-16, p. 102
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    v = v | (v >> 8);
    v = v | (v >> 16);
    return qPopulationCount(~v);
#endif
}

 inline uint qCountLeadingZeroBits(quint8 v) throw()
{
#if defined(QT_HAS_BUILTIN_CLZ)
    return v ? QAlgorithmsPrivate::qt_builtin_clz(v)-24U : 8U;
#else
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    return qPopulationCount(static_cast<quint8>(~v));
#endif
}

 inline uint qCountLeadingZeroBits(quint16 v) throw()
{
#if defined(QT_HAS_BUILTIN_CLZS)
    return v ? QAlgorithmsPrivate::qt_builtin_clzs(v) : 16U;
#else
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    v = v | (v >> 8);
    return qPopulationCount(static_cast<quint16>(~v));
#endif
}

 inline uint qCountLeadingZeroBits(quint64 v) throw()
{
#if defined(QT_HAS_BUILTIN_CLZLL)
    return v ? QAlgorithmsPrivate::qt_builtin_clzll(v) : 64U;
#else
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    v = v | (v >> 8);
    v = v | (v >> 16);
    v = v | (v >> 32);
    return qPopulationCount(~v);
#endif
}

 inline uint qCountLeadingZeroBits(unsigned long v) throw()
{
    return qCountLeadingZeroBits(QIntegerForSizeof<long>::Unsigned(v));
}

#endif // QALGORITHMS_H
