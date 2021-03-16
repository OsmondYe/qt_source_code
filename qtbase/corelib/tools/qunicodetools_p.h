#ifndef QUNICODETOOLS_P_H
#define QUNICODETOOLS_P_H

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qchar.h>

struct QCharAttributes
{
    uchar graphemeBoundary : 1;
    uchar wordBreak        : 1;
    uchar sentenceBoundary : 1;
    uchar lineBreak        : 1;
    uchar whiteSpace       : 1;
    uchar wordStart        : 1;
    uchar wordEnd          : 1;
    uchar mandatoryBreak   : 1;
};
//Q_DECLARE_TYPEINFO(QCharAttributes, Q_PRIMITIVE_TYPE);

namespace QUnicodeTools {

// ### temporary
struct ScriptItem
{
    int position;
    int script;
};

} // namespace QUnicodeTools
//Q_DECLARE_TYPEINFO(QUnicodeTools::ScriptItem, Q_PRIMITIVE_TYPE);
namespace QUnicodeTools {

enum CharAttributeOption {
    GraphemeBreaks = 0x01,
    WordBreaks = 0x02,
    SentenceBreaks = 0x04,
    LineBreaks = 0x08,
    WhiteSpaces = 0x10,
    DefaultOptionsCompat = GraphemeBreaks | LineBreaks | WhiteSpaces, // ### remove

    DontClearAttributes = 0x1000
};
//Q_DECLARE_FLAGS(CharAttributeOptions, CharAttributeOption)
typedef QFlags<CharAttributeOption> CharAttributeOptions;


// attributes buffer has to have a length of string length + 1
 void initCharAttributes(const ushort *string, int length,
                                      const ScriptItem *items, int numItems,
                                      QCharAttributes *attributes, CharAttributeOptions options = DefaultOptionsCompat);


 void initScripts(const ushort *string, int length, uchar *scripts);

} // namespace QUnicodeTools

#endif // QUNICODETOOLS_P_H
