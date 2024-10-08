/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2014 Dan Leinir Turthra Jensen <admin@leinir.dk>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "ParagraphStylesModel.h"
#include <KWDocument.h>
#include <KoParagraphStyle.h>
#include <KoStyleManager.h>
#include <KoTextEditor.h>

class ParagraphStylesModel::Private
{
public:
    Private(ParagraphStylesModel *qq)
        : q(qq)
        , document(nullptr)
    {
    }
    ParagraphStylesModel *q;
    QPointer<KWDocument> document;
    QList<KoParagraphStyle *> styles;
    QPointer<KoTextEditor> textEditor;
    KoParagraphStyle *cursorStyle;
    QFont cursorFont;
    qreal zoomLevel;

    void updateStylesList()
    {
        q->beginResetModel();
        styles.clear();
        if (document) {
            KoStyleManager *styleManager = document->resourceManager()->resource(KoText::StyleManager).value<KoStyleManager *>();
            QList<KoParagraphStyle *> paragraphStyles = styleManager->paragraphStyles();
            KoParagraphStyle *defaultParagraphStyle = styleManager->defaultParagraphStyle();
            foreach (KoParagraphStyle *style, paragraphStyles) {
                if (style != defaultParagraphStyle) {
                    styles.append(style);
                }
            }
        }
        q->endResetModel();
    }
};

ParagraphStylesModel::ParagraphStylesModel()
    : d(new Private(this))
{
}

ParagraphStylesModel::~ParagraphStylesModel()
{
    delete d;
}

QHash<int, QByteArray> ParagraphStylesModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Name] = "name";
    roleNames[Current] = "current";
    roleNames[Font] = "font";
    roleNames[FontFamily] = "fontFamily";
    roleNames[FontPointSize] = "fontPointSize";
    roleNames[FontWeight] = "fontWeight";
    roleNames[FontItalic] = "fontItalic";
    roleNames[FontUnderline] = "fontUnderline";
    return roleNames;
}

QVariant ParagraphStylesModel::data(const QModelIndex &index, int role) const
{
    QVariant data;
    if (index.isValid() && index.row() < d->styles.count() && d->document) {
        KoCharacterStyle *style = d->styles.at(index.row());
        QFont font = style->font();
        switch (role) {
        case Name:
            data.setValue(style->name());
            break;
        case Current:
            data.setValue(style == d->cursorStyle);
            break;
        case Font:
            font.setUnderline((style->underlineStyle() != KoCharacterStyle::NoLineStyle));
            font.setPointSize(font.pointSize() * d->zoomLevel);
            data.setValue(font);
            break;
        case FontFamily:
            data.setValue(style->fontFamily());
            break;
        case FontPointSize:
            data.setValue(style->fontPointSize() * d->zoomLevel);
            break;
        case FontItalic:
            data.setValue(style->fontItalic());
            break;
        case FontWeight:
            data.setValue(style->fontWeight());
            break;
        case FontUnderline:
            data.setValue((style->underlineStyle() != KoCharacterStyle::NoLineStyle));
            break;
        default:
            data.setValue(QString("Unknown role"));
            break;
        }
    }
    return data;
}

int ParagraphStylesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return d->styles.count();
}

QObject *ParagraphStylesModel::document() const
{
    return d->document;
}

void ParagraphStylesModel::setDocument(QObject *newDocument)
{
    d->document = qobject_cast<KWDocument *>(newDocument);
    d->updateStylesList();
    emit documentChanged();
}

QObject *ParagraphStylesModel::textEditor() const
{
    return d->textEditor;
}

void ParagraphStylesModel::setTextEditor(QObject *newEditor)
{
    if (d->textEditor)
        d->textEditor->disconnect(this);
    d->textEditor = qobject_cast<KoTextEditor *>(newEditor);
    if (d->textEditor)
        connect(d->textEditor.data(), &KoTextEditor::cursorPositionChanged, this, &ParagraphStylesModel::cursorPositionChanged);
    emit textEditorChanged();
}

void ParagraphStylesModel::cursorPositionChanged()
{
    QTextBlockFormat bf = d->textEditor->blockFormat();
    d->cursorStyle =
        d->document->resourceManager()->resource(KoText::StyleManager).value<KoStyleManager *>()->paragraphStyle(bf.intProperty(KoParagraphStyle::StyleId));
    dataChanged(index(0), index(d->styles.count() - 1));

    QTextCharFormat charFormat = d->textEditor->charFormat();
    d->cursorFont = charFormat.font();
    emit cursorFontChanged();
}

void ParagraphStylesModel::activate(int index)
{
    if (d->textEditor && index > -1 && index < d->styles.count()) {
        KoParagraphStyle *style = d->styles.at(index);
        if (style == d->cursorStyle)
            return;
        d->textEditor->setStyle(style);
        cursorPositionChanged();
    }
}

QFont ParagraphStylesModel::cursorFont() const
{
    return d->cursorFont;
}

int ParagraphStylesModel::currentStyle() const
{
    return d->styles.indexOf(d->cursorStyle);
}

qreal ParagraphStylesModel::zoomLevel() const
{
    return d->zoomLevel;
}

void ParagraphStylesModel::setZoomLevel(const qreal &newZoom)
{
    d->zoomLevel = newZoom;
    if (d->styles.count() > 0)
        dataChanged(index(0), index(d->styles.count() - 1));
    emit zoomLevelChanged();
}
