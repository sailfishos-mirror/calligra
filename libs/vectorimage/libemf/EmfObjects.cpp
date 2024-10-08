/*
  SPDX-FileCopyrightText: 2008 Brad Hards <bradh@frogmouth.net>
  SPDX-FileCopyrightText: 2009, 2011 Inge Wallin <inge@lysator.liu.se>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "EmfObjects.h"

#include <VectorImageDebug.h>

namespace
{

// Routine to throw away a specific number of bytes
void soakBytes(QDataStream &stream, quint32 numBytes)
{
    quint8 scratch;
    for (quint32 i = 0; i < numBytes; ++i) {
        stream >> scratch;
    }
}

// Convenience function to handle a 2-byte wide char stream
QString recordWChars(QDataStream &stream, quint32 numChars)
{
    QString text;
    QChar myChar;
    for (quint32 i = 0; i < numChars; ++i) {
        stream >> myChar;
        text.append(myChar);
    }
    return text;
}

// Convenience function to handle a 1-byte wide char stream
QString recordChars(QDataStream &stream, quint32 numChars)
{
    QString text;
    quint8 myChar;
    for (quint32 i = 0; i < numChars; ++i) {
        stream >> myChar;
        text.append(QChar(myChar));
    }
    return text;
}
}

namespace Libemf
{

// ================================================================
//                         class EmrTextObject
//

// See MS-EMF section 2.2.5.

EmrTextObject::EmrTextObject(QDataStream &stream, quint32 size, TextType textType)
{
    stream >> m_referencePoint;
    size -= 8;
    // debugVectorImage << "Text ref. point:" << m_referencePoint;

    stream >> m_charCount;
    size -= 4;
    // debugVectorImage << "Number of characters in string:" << m_charCount;

    stream >> m_offString;
    size -= 4;

    // 36 bytes for the body of the parent structure (EMR_EXTTEXTOUTA or EMR_EXTTEXTOUTW)
    // then parts of the EmrText structure
    quint32 offString = m_offString - 36 - 8 - 4 - 4;

    stream >> m_options;
    size -= 4;
    offString -= 4;

    stream >> m_rectangle;
    size -= 16;
    offString -= 16;

    stream >> m_offDx;
    size -= 4;
    offString -= 4;
    // as for offString. 36 bytes for parent, then the earlier parts of EmrText

    soakBytes(stream, offString); // skips over UndefinedSpace1.
    size -= offString;

    if (textType == SixteenBitChars) {
        m_textString = recordWChars(stream, m_charCount);
        size -= 2 * m_charCount;

        // If the number of characters is uneven, then we need to soak 2
        // bytes to make it a full word.
        if (m_charCount & 0x01) {
            soakBytes(stream, 2);
            size -= 2;
        }
    } else {
        m_textString = recordChars(stream, m_charCount);
        size -= m_charCount;

        // If the number of characters is not a multiple of 4, then we need to soak some
        // bytes to make it a full word.
        quint32 rest = m_charCount % 4;
        if (rest != 0) {
            soakBytes(stream, 4 - rest);
            size -= 4 - rest;
        }
    }

    // TODO: parse the spacing array
    soakBytes(stream, size);
}

EmrTextObject::~EmrTextObject() = default;

QPoint EmrTextObject::referencePoint() const
{
    return m_referencePoint;
}

QString EmrTextObject::textString() const
{
    return m_textString;
}

quint32 EmrTextObject::options() const
{
    return m_options;
}

QRect EmrTextObject::rectangle() const
{
    return m_rectangle;
}
}
