/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KPrPlaceholders.h"

#include "KPrPageLayout.h"
#include "KPrPlaceholder.h"
#include "KPrPlaceholderShape.h"
#include "StageDebug.h"
#include "commands/KPrPageLayoutCommand.h"

#include <KoPADocument.h>
#include <KoParagraphStyle.h>
#include <KoShape.h>
#include <KoShapeContainer.h>
#include <KoShapeCreateCommand.h>
#include <KoShapeDeleteCommand.h>
#include <KoShapeLayer.h>
#include <KoShapeMoveCommand.h>
#include <KoShapeSizeCommand.h>
#include <KoStyleManager.h>
#include <KoTextDocument.h>
#include <KoTextShapeData.h>

#include <QTextCursor>
#include <QTextDocument>

KPrPlaceholders::KPrPlaceholders()
    : m_layout(nullptr)
{
}

KPrPlaceholders::~KPrPlaceholders() = default;

void KPrPlaceholders::setLayout(KPrPageLayout *layout,
                                KoPADocument *document,
                                const QList<KoShape *> &shapes,
                                const QSizeF &pageSize,
                                const QMap<QString, KoTextShapeData *> &styles)
{
    Q_ASSERT(m_initialized);

    document->beginMacro(kundo2_i18n("Set Layout")); // we call applyStyle() which creates undo commands too.
    KUndo2Command *cmd = new KUndo2Command(kundo2_i18n("Set Layout"));
    new KPrPageLayoutCommand(this, layout, cmd);

    Q_ASSERT(!shapes.isEmpty());
    KoShapeLayer *layer = dynamic_cast<KoShapeLayer *>(shapes[0]);

    QMap<QString, QVector<QRectF>> placeholders;
    if (layout) {
        foreach (KPrPlaceholder *placeholder, layout->placeholders()) {
            placeholders[placeholder->presentationObject()].append(placeholder->rect(pageSize));
        }
    }

    Placeholders::iterator it(m_placeholders.begin());

    while (it != m_placeholders.end()) {
        Placeholders::iterator next(it);
        ++next;
        QMap<QString, QVector<QRectF>>::iterator itPlaceholder(placeholders.find(it->presentationClass));
        // modify existing placeholders to get the position and size defined in the new layout
        if (itPlaceholder != placeholders.end() && !itPlaceholder.value().isEmpty()) {
            QRectF rect = itPlaceholder.value().takeFirst();
            if (itPlaceholder.value().isEmpty()) {
                placeholders.erase(itPlaceholder);
            }
            // replace the shape as given by the layout
            QList<KoShape *> modifiedShape;
            QVector<QSizeF> oldSize;
            QVector<QSizeF> newSize;
            QVector<QPointF> oldPosition;
            QVector<QPointF> newPosition;
            modifiedShape.append(it->shape);
            oldSize.append(it->shape->size());
            newSize.append(rect.size());
            oldPosition.append(it->shape->position());
            newPosition.append(rect.topLeft());
            new KoShapeSizeCommand(modifiedShape, oldSize, newSize, cmd);
            new KoShapeMoveCommand(modifiedShape, oldPosition, newPosition, cmd);
        } else {
            // remove no longer used placeholders
            if (it->isPlaceholder) {
                // shape remove
                // this is done as it gets deleted by shapeRemoved
                Placeholders::iterator next(it);
                ++next;
                new KoShapeDeleteCommand(document, it->shape, cmd);
            }
        }
        it = next;
    }

    // add placeholder shapes for all available positions
    QMap<QString, QVector<QRectF>>::const_iterator itPlaceholder(placeholders.constBegin());
    for (; itPlaceholder != placeholders.constEnd(); ++itPlaceholder) {
        const QVector<QRectF> &list(itPlaceholder.value());
        QVector<QRectF>::const_iterator listIt(list.begin());
        for (; listIt != list.end(); ++listIt) {
            KPrPlaceholderShape *shape = new KPrPlaceholderShape(itPlaceholder.key());
            shape->initStrategy(document->resourceManager());
            shape->setAdditionalAttribute("presentation:placeholder", "true");
            shape->setAdditionalAttribute("presentation:class", itPlaceholder.key());
            shape->setSize((*listIt).size());
            shape->setPosition((*listIt).topLeft());
            shape->setParent(layer);
            shape->setShapeId(KPrPlaceholderShapeId);
            applyStyle(shape, itPlaceholder.key(), styles);
            new KoShapeCreateCommand(document, shape, cmd);
        }
    }
    document->addCommand(cmd);
    document->endMacro();
}

void KPrPlaceholders::setLayout(KPrPageLayout *layout)
{
    Q_ASSERT(m_initialized);
    m_layout = layout;
}

void KPrPlaceholders::init(KPrPageLayout *layout, const QList<KoShape *> &shapes)
{
    m_layout = layout;
    add(shapes);

    m_initialized = true;
}

KPrPageLayout *KPrPlaceholders::layout() const
{
    Q_ASSERT(m_initialized);
    return m_layout;
}

void KPrPlaceholders::shapeAdded(KoShape *shape)
{
    Q_ASSERT(m_initialized);
    // if presentation:class add to index no matter if it is a placeholder or not
    QString presentationClass = shape->additionalAttribute("presentation:class");
    QString placeholder = shape->additionalAttribute("presentation:placeholder");
    if (!presentationClass.isNull()) {
        m_placeholders.get<1>().insert(Placeholder(presentationClass, shape, placeholder == "true"));
    }
}

void KPrPlaceholders::shapeRemoved(KoShape *shape)
{
    Q_ASSERT(m_initialized);
    // if it is a placeholder remove it
    // if presentation:class is set and not a placeholder remove it and add a placeholder
    //    this needs to be checked as on undo/redo we might get a problem
    // other do nothing
    QString presentationClass = shape->additionalAttribute("presentation:class");
    if (!presentationClass.isNull()) {
        PlaceholdersByShape::iterator it(m_placeholders.get<2>().find(shape));
        if (it != m_placeholders.get<2>().end()) {
            m_placeholders.get<2>().erase(it);
        }
    }
}

void KPrPlaceholders::add(const QList<KoShape *> &shapes)
{
    foreach (KoShape *shape, shapes) {
        QString presentationClass = shape->additionalAttribute("presentation:class");
        QString placeholder = shape->additionalAttribute("presentation:placeholder");
        if (!presentationClass.isNull()) {
            m_placeholders.get<1>().insert(Placeholder(presentationClass, shape, placeholder == "true"));
        }
        KoShapeContainer *container = dynamic_cast<KoShapeContainer *>(shape);
        if (container) {
            add(container->shapes());
        }
    }
}

void KPrPlaceholders::debug() const
{
    debugStage << "size" << m_placeholders.size() << "init:" << m_initialized;
    Placeholders::iterator it(m_placeholders.begin());
    for (; it != m_placeholders.end(); ++it) {
        debugStage << "placeholder" << it->presentationClass << it->shape << it->shape->shapeId() << it->isPlaceholder;
    }
}

QMap<QString, KoTextShapeData *> KPrPlaceholders::styles() const
{
    QMap<QString, KoTextShapeData *> styles;
    Placeholders::iterator it(m_placeholders.begin());
    for (; it != m_placeholders.end(); ++it) {
        KoTextShapeData *data = nullptr;
        // this is done like that as userData is not virtual
        if (KPrPlaceholderShape *shape = dynamic_cast<KPrPlaceholderShape *>(it->shape)) {
            data = qobject_cast<KoTextShapeData *>(shape->userData());
        } else {
            data = qobject_cast<KoTextShapeData *>(it->shape->userData());
        }
        if (data && !styles.contains(it->presentationClass)) {
            styles.insert(it->presentationClass, data);
        }
    }
    return styles;
}

void KPrPlaceholders::applyStyle(KPrPlaceholderShape *shape, const QString &presentationClass, const QMap<QString, KoTextShapeData *> &styles)
{
    // use outline as fallback
    KoTextShapeData *data = styles.value(presentationClass, 0);
    if (!data) {
        data = styles.value("outline", 0);
    }
    KoTextShapeData *newData = qobject_cast<KoTextShapeData *>(shape->userData());
    debugStage << "data" << data << "newData:" << newData << shape->userData();
    if (data && newData) {
        debugStage << "apply";
        QTextCursor cursor(data->document());
        QTextCursor newCursor(newData->document());
        newCursor.select(QTextCursor::Document);
        KoTextDocument textDocument(newData->document());

        QTextBlockFormat blockFormat(cursor.blockFormat());
        // TODO check if needed
        // blockFormat.setProperty( KoParagraphStyle::StyleId, styleManager->defaultParagraphStyle()->styleId() );
        blockFormat.clearProperty(QTextFormat::ObjectIndex);
        newCursor.setBlockFormat(blockFormat);

        QTextCharFormat chatFormat(cursor.blockCharFormat());
        // TODO check if needed
        // chatFormat.setProperty( KoCharacterStyle::StyleId, styleManager->defaultParagraphStyle()->characterStyle()->styleId() );
        chatFormat.clearProperty(QTextFormat::ObjectIndex);
        newCursor.select(QTextCursor::Document);
        newCursor.setBlockCharFormat(chatFormat);
        newCursor.setCharFormat(chatFormat);
    }
}
