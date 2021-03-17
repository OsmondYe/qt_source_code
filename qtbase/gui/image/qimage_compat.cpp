#define QT_COMPILING_QIMAGE_COMPAT_CPP

#include "qimage.h"


// These implementations must be the same as the inline versions in qimage.h

QImage QImage::convertToFormat(Format f, Qt::ImageConversionFlags flags) const
{
    return convertToFormat_helper(f, flags);
}

QImage QImage::mirrored(bool horizontally, bool vertically) const
{
    return mirrored_helper(horizontally, vertically);
}

QImage QImage::rgbSwapped() const
{
    return rgbSwapped_helper();
}

