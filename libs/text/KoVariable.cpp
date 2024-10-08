/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006-2009 Thomas Zander <zander@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "KoVariable.h"

#include "KoInlineObject_p.h"

#include <KoShape.h>

#include "TextDebug.h"
#include <QFontMetricsF>
#include <QPainter>
#include <QTextDocument>
#include <QTextInlineObject>

class KoVariablePrivate : public KoInlineObjectPrivate
{
public:
    KoVariablePrivate()
        : modified(true)
        , document(nullptr)
        , lastPositionInDocument(-1)
    {
    }

    QDebug printDebug(QDebug dbg) const override
    {
        dbg.nospace() << "KoVariable value=" << value;
        return dbg.space();
    }

    QString value;
    bool modified;
    const QTextDocument *document;
    int lastPositionInDocument;
};

KoVariable::KoVariable(bool propertyChangeListener)
    : KoInlineObject(*(new KoVariablePrivate()), propertyChangeListener)
{
}

KoVariable::~KoVariable() = default;

void KoVariable::setValue(const QString &value)
{
    Q_D(KoVariable);
    if (d->value == value)
        return;
    d->value = value;
    d->modified = true;
    if (d->document) {
        const_cast<QTextDocument *>(d->document)->markContentsDirty(d->lastPositionInDocument, 0);
    }
}

void KoVariable::updatePosition(const QTextDocument *document, int posInDocument, const QTextCharFormat &format)
{
    Q_D(KoVariable);
    if (d->document) {
        disconnect(d->document, &QObject::destroyed, this, &KoVariable::documentDestroyed);
    }
    d->document = document;
    connect(d->document, &QObject::destroyed, this, &KoVariable::documentDestroyed);
    d->lastPositionInDocument = posInDocument;
    Q_UNUSED(format);
    // Variables are always 'in place' so the position is 100% defined by the text layout.
    variableMoved(d->document, posInDocument);
}

void KoVariable::resize(const QTextDocument *document, QTextInlineObject &object, int posInDocument, const QTextCharFormat &format, QPaintDevice *pd)
{
    Q_D(KoVariable);
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);
    if (d->modified == false)
        return;
    if (object.isValid() == false)
        return;
    d->modified = true;
    Q_ASSERT(format.isCharFormat());
    QFontMetricsF fm(format.font(), pd);

    qreal width = qMax(qreal(0.0), fm.boundingRect(d->value).width());
    qreal ascent = fm.ascent();
    qreal descent = fm.descent();
    if (object.width() != width) {
        object.setWidth(width);
    }
    if (object.ascent() != ascent) {
        object.setAscent(ascent);
    }
    if (object.descent() != descent) {
        object.setDescent(descent);
    }
}

void KoVariable::paint(QPainter &painter,
                       QPaintDevice *pd,
                       const QTextDocument *document,
                       const QRectF &rect,
                       const QTextInlineObject &object,
                       int posInDocument,
                       const QTextCharFormat &format)
{
    Q_D(KoVariable);
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);

    // TODO set all the font properties from the format (color etc)
    QFont font(format.font(), pd);
    QTextLayout layout(d->value, font, pd);
    layout.setCacheEnabled(true);
    QVector<QTextLayout::FormatRange> layouts;
    QTextLayout::FormatRange range;
    range.start = 0;
    range.length = d->value.length();
    range.format = format;
    layouts.append(range);
    layout.setFormats(layouts);

    QTextOption option(Qt::AlignLeft | Qt::AlignAbsolute);
    if (object.isValid()) {
        option.setTextDirection(object.textDirection());
    }
    layout.setTextOption(option);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();
    layout.draw(&painter, rect.topLeft());
}

void KoVariable::variableMoved(const QTextDocument *document, int posInDocument)
{
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);
}

QString KoVariable::value() const
{
    Q_D(const KoVariable);
    return d->value;
}

int KoVariable::positionInDocument() const
{
    Q_D(const KoVariable);
    return d->lastPositionInDocument;
}

void KoVariable::documentDestroyed()
{
    // deleteLater(); does not work when closing a document as the inline object manager is deleted before the control is given back to the event loop
    // therefore commit suicide.
    // See http://www.parashift.com/c++-faq-lite/delete-this.html
    delete (this);
}
