// oye 我删去了 operator >>  <<  < > 这些意义都能猜出来
#ifndef QKEYSEQUENCE_H
#define QKEYSEQUENCE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qobjectdefs.h>

QT_BEGIN_NAMESPACE


class QKeySequence;

void qt_set_sequence_auto_mnemonic(bool b);

class QVariant;
class QKeySequencePrivate;

uint qHash(const QKeySequence &key, uint seed = 0) ;

// 在 shortcut的context下才有意义
class  QKeySequence
{
    //Q_GADGET
    
	QKeySequencePrivate *d;
public:

    QKeySequence();
    QKeySequence(const QString &key, SequenceFormat format = NativeText);
    QKeySequence(int k1, int k2 = 0, int k3 = 0, int k4 = 0);
    QKeySequence(const QKeySequence &ks);
    QKeySequence(StandardKey key);
    ~QKeySequence();

    int count() const;
    bool isEmpty() const;



    QString toString(SequenceFormat format = PortableText) const;
    static QKeySequence fromString(const QString &str, SequenceFormat format = PortableText);

    static QList<QKeySequence> listFromString(const QString &str, SequenceFormat format = PortableText);
    static QString listToString(const QList<QKeySequence> &list, SequenceFormat format = PortableText);

    SequenceMatch matches(const QKeySequence &seq) const;
    static QKeySequence mnemonic(const QString &text);
    static QList<QKeySequence> keyBindings(StandardKey key);

    operator QVariant() const;
    int operator[](uint i) const;
    QKeySequence &operator=(const QKeySequence &other);
    void swap(QKeySequence &other)  { qSwap(d, other.d); }

    bool operator==(const QKeySequence &other) const;
    inline bool operator!= (const QKeySequence &other) const
    { return !(*this == other); }
    bool operator< (const QKeySequence &ks) const;
    inline bool operator> (const QKeySequence &other) const
    { return other < *this; }
    inline bool operator<= (const QKeySequence &other) const
    { return !(other < *this); }
    inline bool operator>= (const QKeySequence &other) const
    { return !(*this < other); }

    bool isDetached() const;
private:
    static int decodeString(const QString &ks);
    static QString encodeString(int key);
    int assign(const QString &str);
    int assign(const QString &str, SequenceFormat format);
    void setKey(int key, int index);



    friend  QDataStream &operator<<(QDataStream &in, const QKeySequence &ks);
    friend  QDataStream &operator>>(QDataStream &in, QKeySequence &ks);
    friend  uint qHash(const QKeySequence &key, uint seed) ;
    friend class QShortcutMap;
    friend class QShortcut;

public:
    typedef QKeySequencePrivate * DataPtr;
    inline DataPtr &data_ptr() { return d; }



    enum StandardKey {
        UnknownKey,
        HelpContents,
        WhatsThis,
        Open,
        Close,
        Save,
        New,
        Delete,
        Cut,
        Copy,
        Paste,
        Undo,
        Redo,
        Back,
        Forward,
        Refresh,
        ZoomIn,
        ZoomOut,
        Print,
        AddTab,
        NextChild,
        PreviousChild,
        Find,
        FindNext,
        FindPrevious,
        Replace,
        SelectAll,
        Bold,
        Italic,
        Underline,
        MoveToNextChar,
        MoveToPreviousChar,
        MoveToNextWord,
        MoveToPreviousWord,
        MoveToNextLine,
        MoveToPreviousLine,
        MoveToNextPage,
        MoveToPreviousPage,
        MoveToStartOfLine,
        MoveToEndOfLine,
        MoveToStartOfBlock,
        MoveToEndOfBlock,
        MoveToStartOfDocument,
        MoveToEndOfDocument,
        SelectNextChar,
        SelectPreviousChar,
        SelectNextWord,
        SelectPreviousWord,
        SelectNextLine,
        SelectPreviousLine,
        SelectNextPage,
        SelectPreviousPage,
        SelectStartOfLine,
        SelectEndOfLine,
        SelectStartOfBlock,
        SelectEndOfBlock,
        SelectStartOfDocument,
        SelectEndOfDocument,
        DeleteStartOfWord,
        DeleteEndOfWord,
        DeleteEndOfLine,
        InsertParagraphSeparator,
        InsertLineSeparator,
        SaveAs,
        Preferences,
        Quit,
        FullScreen,
        Deselect,
        DeleteCompleteLine,
        Backspace,
        Cancel						// oye 窗口关闭时 接受键盘的cancel
     };
     //Q_ENUM(StandardKey)

    enum SequenceFormat {
        NativeText,
        PortableText
    };

    enum SequenceMatch {
    NoMatch,
    PartialMatch,
    ExactMatch
	};
	
};

//Q_DECLARE_SHARED(QKeySequence)
QT_END_NAMESPACE

#endif // QKEYSEQUENCE_H
