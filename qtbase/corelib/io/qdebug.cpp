#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif
#ifdef qDebug
#undef qDebug
#endif

#include "qdebug.h"
#include "qmetaobject.h"
#include <private/qtextstream_p.h>
#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE

using QtMiscUtils::toHexUpper;
using QtMiscUtils::fromHex;


QDebug::~QDebug()
{
    if (!--stream->ref) {
        if (stream->space && stream->buffer.endsWith(QLatin1Char(' ')))
            stream->buffer.chop(1);
        if (stream->message_output) {
            qt_message_output(stream->type,
                              stream->context,
                              stream->buffer);
        }
        delete stream;
    }
}

/*!
    \internal
*/
void QDebug::putUcs4(uint ucs4)
{
    maybeQuote('\'');
    if (ucs4 < 0x20) {
        stream->ts << "\\x" << hex << ucs4 << reset;
    } else if (ucs4 < 0x80) {
        stream->ts << char(ucs4);
    } else {
        if (ucs4 < 0x10000)
            stream->ts << "\\u" << qSetFieldWidth(4);
        else
            stream->ts << "\\U" << qSetFieldWidth(8);
        stream->ts << hex << qSetPadChar(QLatin1Char('0')) << ucs4 << reset;
    }
    maybeQuote('\'');
}

// These two functions return true if the character should be printed by QDebug.
// For QByteArray, this is technically identical to US-ASCII isprint();
// for QString, we use QChar::isPrint, which requires a full UCS-4 decode.
static inline bool isPrintable(uint ucs4)
{ return QChar::isPrint(ucs4); }
static inline bool isPrintable(ushort uc)
{ return QChar::isPrint(uc); }
static inline bool isPrintable(uchar c)
{ return c >= ' ' && c < 0x7f; }

template <typename Char>
static inline void putEscapedString(QTextStreamPrivate *d, const Char *begin, int length, bool isUnicode = true)
{
    QChar quote(QLatin1Char('"'));
    d->write(&quote, 1);

    bool lastWasHexEscape = false;
    const Char *end = begin + length;
    for (const Char *p = begin; p != end; ++p) {
        // check if we need to insert "" to break an hex escape sequence
        if (Q_UNLIKELY(lastWasHexEscape)) {
            if (fromHex(*p) != -1) {
                // yes, insert it
                QChar quotes[] = { QLatin1Char('"'), QLatin1Char('"') };
                d->write(quotes, 2);
            }
            lastWasHexEscape = false;
        }

        if (sizeof(Char) == sizeof(QChar)) {
            // Surrogate characters are category Cs (Other_Surrogate), so isPrintable = false for them
            int runLength = 0;
            while (p + runLength != end &&
                   isPrintable(p[runLength]) && p[runLength] != '\\' && p[runLength] != '"')
                ++runLength;
            if (runLength) {
                d->write(reinterpret_cast<const QChar *>(p), runLength);
                p += runLength - 1;
                continue;
            }
        } else if (isPrintable(*p) && *p != '\\' && *p != '"') {
            QChar c = QLatin1Char(*p);
            d->write(&c, 1);
            continue;
        }

        // print as an escape sequence (maybe, see below for surrogate pairs)
        int buflen = 2;
        ushort buf[sizeof "\\U12345678" - 1];
        buf[0] = '\\';

        switch (*p) {
        case '"':
        case '\\':
            buf[1] = *p;
            break;
        case '\b':
            buf[1] = 'b';
            break;
        case '\f':
            buf[1] = 'f';
            break;
        case '\n':
            buf[1] = 'n';
            break;
        case '\r':
            buf[1] = 'r';
            break;
        case '\t':
            buf[1] = 't';
            break;
        default:
            if (!isUnicode) {
                // print as hex escape
                buf[1] = 'x';
                buf[2] = toHexUpper(uchar(*p) >> 4);
                buf[3] = toHexUpper(uchar(*p));
                buflen = 4;
                lastWasHexEscape = true;
                break;
            }
            if (QChar::isHighSurrogate(*p)) {
                if ((p + 1) != end && QChar::isLowSurrogate(p[1])) {
                    // properly-paired surrogates
                    uint ucs4 = QChar::surrogateToUcs4(*p, p[1]);
                    if (isPrintable(ucs4)) {
                        buf[0] = *p;
                        buf[1] = p[1];
                        buflen = 2;
                    } else {
                        buf[1] = 'U';
                        buf[2] = '0'; // toHexUpper(ucs4 >> 32);
                        buf[3] = '0'; // toHexUpper(ucs4 >> 28);
                        buf[4] = toHexUpper(ucs4 >> 20);
                        buf[5] = toHexUpper(ucs4 >> 16);
                        buf[6] = toHexUpper(ucs4 >> 12);
                        buf[7] = toHexUpper(ucs4 >> 8);
                        buf[8] = toHexUpper(ucs4 >> 4);
                        buf[9] = toHexUpper(ucs4);
                        buflen = 10;
                    }
                    ++p;
                    break;
                }
                // improperly-paired surrogates, fall through
            }
            buf[1] = 'u';
            buf[2] = toHexUpper(ushort(*p) >> 12);
            buf[3] = toHexUpper(ushort(*p) >> 8);
            buf[4] = toHexUpper(*p >> 4);
            buf[5] = toHexUpper(*p);
            buflen = 6;
        }
        d->write(reinterpret_cast<QChar *>(buf), buflen);
    }

    d->write(&quote, 1);
}

/*!
    \internal
    Duplicated from QtTest::toPrettyUnicode().
*/
void QDebug::putString(const QChar *begin, size_t length)
{
    if (stream->testFlag(Stream::NoQuotes)) {
        // no quotes, write the string directly too (no pretty-printing)
        // this respects the QTextStream state, though
        stream->ts.d_ptr->putString(begin, int(length));
    } else {
        // we'll reset the QTextStream formatting mechanisms, so save the state
        QDebugStateSaver saver(*this);
        stream->ts.d_ptr->params.reset();
        putEscapedString(stream->ts.d_ptr.data(), reinterpret_cast<const ushort *>(begin), int(length));
    }
}

/*!
    \internal
    Duplicated from QtTest::toPrettyCString().
*/
void QDebug::putByteArray(const char *begin, size_t length, Latin1Content content)
{
    if (stream->testFlag(Stream::NoQuotes)) {
        // no quotes, write the string directly too (no pretty-printing)
        // this respects the QTextStream state, though
        QString string = content == ContainsLatin1 ? QString::fromLatin1(begin, int(length)) : QString::fromUtf8(begin, int(length));
        stream->ts.d_ptr->putString(string);
    } else {
        // we'll reset the QTextStream formatting mechanisms, so save the state
        QDebugStateSaver saver(*this);
        stream->ts.d_ptr->params.reset();
        putEscapedString(stream->ts.d_ptr.data(), reinterpret_cast<const uchar *>(begin),
                         int(length), content == ContainsLatin1);
    }
}

/*!
    \fn QDebug::swap(QDebug &other)
    \since 5.0

    Swaps this debug stream instance with \a other. This function is
    very fast and never fails.
*/

/*!
    Resets the stream formatting options, bringing it back to its original constructed state.

    \sa space(), quote()
    \since 5.4
*/
QDebug &QDebug::resetFormat()
{
    stream->ts.reset();
    stream->space = true;
    if (stream->context.version > 1)
        stream->flags = 0;
    stream->setVerbosity(Stream::DefaultVerbosity);
    return *this;
}


class QDebugStateSaverPrivate
{
public:
    QDebugStateSaverPrivate(QDebug &dbg)
        : m_dbg(dbg),
          m_spaces(dbg.autoInsertSpaces()),
          m_flags(0),
          m_streamParams(dbg.stream->ts.d_ptr->params)
    {
        if (m_dbg.stream->context.version > 1)
            m_flags = m_dbg.stream->flags;
    }
    void restoreState()
    {
        const bool currentSpaces = m_dbg.autoInsertSpaces();
        if (currentSpaces && !m_spaces)
            if (m_dbg.stream->buffer.endsWith(QLatin1Char(' ')))
                m_dbg.stream->buffer.chop(1);

        m_dbg.setAutoInsertSpaces(m_spaces);
        m_dbg.stream->ts.d_ptr->params = m_streamParams;
        if (m_dbg.stream->context.version > 1)
            m_dbg.stream->flags = m_flags;

        if (!currentSpaces && m_spaces)
            m_dbg.stream->ts << ' ';
    }

    QDebug &m_dbg;

    // QDebug state
    const bool m_spaces;
    int m_flags;

    // QTextStream state
    const QTextStreamPrivate::Params m_streamParams;
};



QDebugStateSaver::QDebugStateSaver(QDebug &dbg)
    : d(new QDebugStateSaverPrivate(dbg))
{
}


QDebugStateSaver::~QDebugStateSaver()
{
    d->restoreState();
}


void qt_QMetaEnum_flagDebugOperator(QDebug &debug, size_t sizeofT, int value)
{
    qt_QMetaEnum_flagDebugOperator<int>(debug, sizeofT, value);
}


QDebug qt_QMetaEnum_debugOperator(QDebug &dbg, int value, const QMetaObject *meta, const char *name)
{
    QDebugStateSaver saver(dbg);
    QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));
    const char *key = me.valueToKey(value);
    dbg.nospace() << meta->className() << "::" << name << '(';
    if (key)
        dbg << key;
    else
        dbg << value;
    dbg << ')';
    return dbg;
}

QDebug qt_QMetaEnum_flagDebugOperator(QDebug &debug, quint64 value, const QMetaObject *meta, const char *name)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat();
    debug.noquote();
    debug.nospace();
    debug << "QFlags<";
    const QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));
    if (const char *scope = me.scope())
        debug << scope << "::";
    debug << me.name() << ">(" << me.valueToKeys(value) << ')';
    return debug;
}

QT_END_NAMESPACE
