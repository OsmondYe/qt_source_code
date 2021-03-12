#ifndef QKEYSEQUENCE_P_H
#define QKEYSEQUENCE_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include "qkeysequence.h"

#include <algorithm>

struct QKeyBinding
{
    QKeySequence::StandardKey standardKey;
    uchar priority;
    uint shortcut;
    uint platform;
};

class QKeySequencePrivate
{
public:
    QAtomicInt ref;
    int key[MaxKeyCount];
    enum { MaxKeyCount = 4 }; // also used in QKeySequenceEdit
    inline QKeySequencePrivate() : ref(1)
    {
        std::fill_n(key, uint(MaxKeyCount), 0);
    }
    inline QKeySequencePrivate(const QKeySequencePrivate &copy) : ref(1)
    {
        std::copy(copy.key, copy.key + MaxKeyCount,
                  QT_MAKE_CHECKED_ARRAY_ITERATOR(key, MaxKeyCount));
    }
    static QString encodeString(int key, QKeySequence::SequenceFormat format);
    // used in dbusmenu
    static QString keyName(int key, QKeySequence::SequenceFormat format);
    static int decodeString(const QString &keyStr, QKeySequence::SequenceFormat format);
};

#endif //QKEYSEQUENCE_P_H
