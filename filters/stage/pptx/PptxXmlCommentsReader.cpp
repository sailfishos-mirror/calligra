/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Carlos Licea <carlos@kdab.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PptxXmlCommentsReader.h"

#include <QPoint>

#define MSOOXML_CURRENT_NS "p"
#define MSOOXML_CURRENT_CLASS PptxCommentsReader

#include <MsooXmlReader_p.h>
#include <MsooXmlUnits.h>
#include <MsooXmlUtils.h>
#include <writeodf/helpers.h>
#include <writeodf/writeodfdc.h>
#include <writeodf/writeodfoffice.h>

using namespace writeodf;

class PptxXmlCommentsReader::Private
{
public:
    int currentComment;

    QMap<int, QString> authors;
    QMap<int, QString> texts;
    QMap<int, QPoint> positions;
    QMap<int, QString> dates;
    PptxXmlCommentsReaderContext *context;
};

PptxXmlCommentsReader::PptxXmlCommentsReader(KoOdfWriters *writers)
    : MsooXmlCommonReader(writers)
    , d(new Private())
{
    d->currentComment = 0;
}

PptxXmlCommentsReader::~PptxXmlCommentsReader()
{
    delete d;
}

KoFilter::ConversionStatus PptxXmlCommentsReader::read(MSOOXML::MsooXmlReaderContext *context)
{
    d->context = dynamic_cast<PptxXmlCommentsReaderContext *>(context);
    Q_ASSERT(d->context);

    readNext();
    if (!isStartDocument()) {
        return KoFilter::WrongFormat;
    }

    readNext();
    KoFilter::ConversionStatus result = read_cmLst();
    Q_ASSERT(result == KoFilter::OK);
    if (result == KoFilter::OK) {
        saveOdfComments();
    }

    return KoFilter::OK;
}

void PptxXmlCommentsReader::saveOdfComments()
{
    for (int i = 0; i < d->currentComment; ++i) {
        office_annotation annotation(body);

        QPoint position = d->positions.value(i);
        // FIXME according to the documentation these measurements are EMUs
        // but I still get wrong values and that's why I multiply by 1500
        const int fixmeFactor = 1500;
        annotation.set_svg_x(EMU_TO_CM_STRING(position.x() * fixmeFactor));
        annotation.set_svg_y(EMU_TO_CM_STRING(position.y() * fixmeFactor));

        annotation.add_dc_creator().addTextNode(d->authors.value(i));
        annotation.add_dc_date().addTextNode(d->dates.value(i));

        text_p p(annotation.add_text_p());
        addTextSpan(p, d->texts.value(i));
    }
}

#undef CURRENT_EL
#define CURRENT_EL cmLst

KoFilter::ConversionStatus PptxXmlCommentsReader::read_cmLst()
{
    READ_PROLOGUE

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(cm)
            ELSE_WRONG_FORMAT
        }
    }

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL cm

KoFilter::ConversionStatus PptxXmlCommentsReader::read_cm()
{
    READ_PROLOGUE

    QXmlStreamAttributes attrs(attributes());

    TRY_READ_ATTR_WITHOUT_NS(authorId)
    const QString author = d->context->authors.value(authorId.toInt());
    d->authors.insert(d->currentComment, author);

    TRY_READ_ATTR_WITHOUT_NS(dt)
    d->dates.insert(d->currentComment, dt);

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(extLst)
            ELSE_TRY_READ_IF(pos)
            ELSE_TRY_READ_IF(text)
            ELSE_WRONG_FORMAT
        }
    }

    d->currentComment++;

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL extLst

KoFilter::ConversionStatus PptxXmlCommentsReader::read_extLst()
{
    READ_PROLOGUE

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            //             TRY_READ_IF(ext)
        }
    }

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL pos

KoFilter::ConversionStatus PptxXmlCommentsReader::read_pos()
{
    READ_PROLOGUE

    QXmlStreamAttributes attrs(attributes());

    READ_ATTR_WITHOUT_NS(x);
    READ_ATTR_WITHOUT_NS(y);

    d->positions.insert(d->currentComment, QPoint(x.toInt(), y.toInt()));

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
    }

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL text

KoFilter::ConversionStatus PptxXmlCommentsReader::read_text()
{
    READ_PROLOGUE

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isCharacters()) {
            d->texts.insert(d->currentComment, text().toString());
        }
    }

    READ_EPILOGUE
}

PptxXmlCommentsReaderContext::PptxXmlCommentsReaderContext()
    : MsooXmlReaderContext()
{
}

PptxXmlCommentsReaderContext::~PptxXmlCommentsReaderContext() = default;
