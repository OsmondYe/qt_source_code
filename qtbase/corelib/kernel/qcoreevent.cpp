#include "qcoreevent.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"

#include "qbasicatomic.h"

#include <limits>

QT_BEGIN_NAMESPACE


QEvent::QEvent(Type type)
    : d(0), t(type), posted(false), spont(false), m_accept(true)
{}


QEvent::QEvent(const QEvent &other)
    : d(other.d), t(other.t), posted(other.posted), spont(other.spont),
      m_accept(other.m_accept)
{
    // if QEventPrivate becomes available, make sure to implement a
    // virtual QEventPrivate *clone() const; function so we can copy here
    Q_ASSERT_X(!d, "QEvent", "Impossible, this can't happen: QEventPrivate isn't defined anywhere");
}


QEvent &QEvent::operator=(const QEvent &other)
{
    // if QEventPrivate becomes available, make sure to implement a
    // virtual QEventPrivate *clone() const; function so we can copy here
    Q_ASSERT_X(!other.d, "QEvent", "Impossible, this can't happen: QEventPrivate isn't defined anywhere");

    t = other.t;
    posted = other.posted;
    spont = other.spont;
    m_accept = other.m_accept;
    return *this;
}


QEvent::~QEvent()
{
    if (posted && QCoreApplication::instance())
        QCoreApplicationPrivate::removePostedEvent(this);
    Q_ASSERT_X(!d, "QEvent", "Impossible, this can't happen: QEventPrivate isn't defined anywhere");
}



namespace {
template <size_t N>
struct QBasicAtomicBitField {
    enum {
        BitsPerInt = std::numeric_limits<uint>::digits,
        NumInts = (N + BitsPerInt - 1) / BitsPerInt,
        NumBits = N
    };

    // This atomic int points to the next (possibly) free ID saving
    // the otherwise necessary scan through 'data':
    QBasicAtomicInteger<uint> next;
    QBasicAtomicInteger<uint> data[NumInts];

    bool allocateSpecific(int which) Q_DECL_NOTHROW
    {
        QBasicAtomicInteger<uint> &entry = data[which / BitsPerInt];
        const uint old = entry.load();
        const uint bit = 1U << (which % BitsPerInt);
        return !(old & bit) // wasn't taken
            && entry.testAndSetRelaxed(old, old | bit); // still wasn't taken

        // don't update 'next' here - it's unlikely that it will need
        // to be updated, in the general case, and having 'next'
        // trailing a bit is not a problem, as it is just a starting
        // hint for allocateNext(), which, when wrong, will just
        // result in a few more rounds through the allocateNext()
        // loop.
    }

    int allocateNext() Q_DECL_NOTHROW
    {
        // Unroll loop to iterate over ints, then bits? Would save
        // potentially a lot of cmpxchgs, because we can scan the
        // whole int before having to load it again.

        // Then again, this should never execute many iterations, so
        // leave like this for now:
        for (uint i = next.load(); i < NumBits; ++i) {
            if (allocateSpecific(i)) {
                // remember next (possibly) free id:
                const uint oldNext = next.load();
                next.testAndSetRelaxed(oldNext, qMax(i + 1, oldNext));
                return i;
            }
        }
        return -1;
    }
};

} // unnamed namespace

typedef QBasicAtomicBitField<QEvent::MaxUser - QEvent::User + 1> UserEventTypeRegistry;

static UserEventTypeRegistry userEventTypeRegistry;

static inline int registerEventTypeZeroBased(int id)
{
    // if the type hint hasn't been registered yet, take it:
    if (id < UserEventTypeRegistry::NumBits && id >= 0 && userEventTypeRegistry.allocateSpecific(id))
        return id;

    // otherwise, ignore hint:
    return userEventTypeRegistry.allocateNext();
}

/*!

    Registers and returns a custom event type. The \a hint provided
    will be used if it is available, otherwise it will return a value
    between QEvent::User and QEvent::MaxUser that has not yet been
    registered. The \a hint is ignored if its value is not between
    QEvent::User and QEvent::MaxUser.

    Returns -1 if all available values are already taken or the
    program is shutting down.
*/
int QEvent::registerEventType(int hint) Q_DECL_NOTHROW
{
    const int result = registerEventTypeZeroBased(QEvent::MaxUser - hint);
    return result < 0 ? -1 : QEvent::MaxUser - result ;
}

QTimerEvent::QTimerEvent(int timerId)
    : QEvent(Timer), id(timerId)
{}

/*!
    \internal
*/
QTimerEvent::~QTimerEvent()
{
}

QChildEvent::QChildEvent(Type type, QObject *child)
    : QEvent(type), c(child)
{}

/*!
    \internal
*/
QChildEvent::~QChildEvent()
{
}

QDynamicPropertyChangeEvent::QDynamicPropertyChangeEvent(const QByteArray &name)
    : QEvent(QEvent::DynamicPropertyChange), n(name)
{
}


QDynamicPropertyChangeEvent::~QDynamicPropertyChangeEvent()
{
}


QDeferredDeleteEvent::QDeferredDeleteEvent()
    : QEvent(QEvent::DeferredDelete)
    , level(0)
{ }


QDeferredDeleteEvent::~QDeferredDeleteEvent()
{ }



QT_END_NAMESPACE

#include "moc_qcoreevent.cpp"
