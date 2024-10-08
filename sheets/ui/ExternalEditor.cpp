/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1999-2006 The KSpread Team <calligra-devel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ExternalEditor.h"

// Sheets
#include "CellEditor.h"
#include "CellToolBase.h"
#include "FormulaEditorHighlighter.h"
#include "core/Map.h"
#include "core/Sheet.h"

// Calligra
#include <KoIcon.h>

// KDE
#include <KLocalizedString>

// Qt
#include <QAction>
#include <QApplication>

using namespace Calligra::Sheets;

class ExternalEditor::Private
{
public:
    CellToolBase *cellTool;
    FormulaEditorHighlighter *highlighter;
    bool isArray;
    QAction *applyAction;
    QAction *cancelAction;
};

ExternalEditor::ExternalEditor(QWidget *parent)
    : KTextEdit(parent)
    , d(new Private)
{
    d->cellTool = nullptr;
    d->highlighter = nullptr;
    d->isArray = false;

    setCurrentFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    // Try to imitate KLineEdit regarding the margins and size.
    document()->setDocumentMargin(1);
    setMinimumHeight(fontMetrics().height() + 2 * frameWidth() + 1);

    connect(this, &QTextEdit::textChanged, this, &ExternalEditor::slotTextChanged);
    connect(this, &QTextEdit::cursorPositionChanged, this, &ExternalEditor::slotCursorPositionChanged);

    d->applyAction = new QAction(koIcon("dialog-ok"), i18n("Apply"), this);
    d->applyAction->setToolTip(i18n("Apply changes"));
    d->applyAction->setEnabled(false);
    connect(d->applyAction, &QAction::triggered, this, &ExternalEditor::applyChanges);

    d->cancelAction = new QAction(koIcon("dialog-cancel"), i18n("Cancel"), this);
    d->cancelAction->setToolTip(i18n("Discard changes"));
    d->cancelAction->setEnabled(false);
    connect(d->cancelAction, &QAction::triggered, this, &ExternalEditor::discardChanges);
}

ExternalEditor::~ExternalEditor()
{
    disconnect(this, nullptr, nullptr, nullptr);
    delete d->highlighter;
    delete d;
}

QSize ExternalEditor::sizeHint() const
{
    return minimumSize();
    // return KTextEdit::sizeHint(); // document()->size().toSize();
}

void ExternalEditor::setCellTool(CellToolBase *cellTool)
{
    if (d->highlighter)
        delete d->highlighter;
    d->cellTool = cellTool;
    d->highlighter = new FormulaEditorHighlighter(this, cellTool->selection());
}

void ExternalEditor::applyChanges()
{
    Q_ASSERT(d->cellTool);
    d->cellTool->deleteEditor(true, d->isArray); // save changes
    d->isArray = false;
}

void ExternalEditor::discardChanges()
{
    Q_ASSERT(d->cellTool);
    clear();
    d->cellTool->deleteEditor(false); // discard changes
    d->cellTool->selection()->update();
}

void ExternalEditor::setText(const QString &text)
{
    Q_ASSERT(d->cellTool);
    if (toPlainText() == text) {
        return;
    }
    // This method is called from the embedded editor. Do not send signals back.
    blockSignals(true);
    KTextEdit::setPlainText(text);
    QTextCursor textCursor = this->textCursor();
    textCursor.setPosition(d->cellTool->editor()->cursorPosition());
    setTextCursor(textCursor);
    blockSignals(false);
}

int ExternalEditor::cursorPosition() const
{
    return textCursor().position();
}

void ExternalEditor::setCursorPosition(int pos)
{
    QTextCursor textCursor(this->textCursor());
    textCursor.setPosition(pos);
    setTextCursor(textCursor);
}

void ExternalEditor::keyPressEvent(QKeyEvent *event)
{
    Q_ASSERT(d->cellTool);
    if (!d->cellTool->selection()->activeSheet()->fullMap()->isReadWrite()) {
        return;
    }

    // Create the embedded editor, if necessary.
    if (!d->cellTool->editor()) {
        d->cellTool->createEditor(false /* keep content */, false /* no focus */, true /*capture arrows */);
    }

    // the Enter and Esc key are handled by the embedded editor
    if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Escape)) {
        d->cellTool->editor()->widget()->setFocus();
        QApplication::sendEvent(d->cellTool->editor()->widget(), event);
        event->accept();
        return;
    }
    // call inherited handler
    KTextEdit::keyPressEvent(event);
}

void ExternalEditor::focusInEvent(QFocusEvent *event)
{
    Q_ASSERT(d->cellTool);
    // If the focussing is user induced.
    if (event->reason() != Qt::OtherFocusReason) {
        debugSheets << "induced by user";
        d->cellTool->setLastEditorWithFocus(CellToolBase::ExternalEditor);
    }
    // when the external editor gets focus, create also the internal editor
    // this in turn means that ranges will be instantly highlighted right
    if (!d->cellTool->editor())
        d->cellTool->createEditor(false /* keep content */, false /* no focus */, true /*capture arrows */);
    KTextEdit::focusInEvent(event);
}

void ExternalEditor::focusOutEvent(QFocusEvent *event)
{
    Q_ASSERT(d->cellTool);
    KTextEdit::focusOutEvent(event);
}

void ExternalEditor::slotTextChanged()
{
    if (!hasFocus())
        return; // only report change if we have focus
    Q_EMIT textModified(toPlainText());
    // Update the cursor position again, because this slot is invoked after
    // slotCursorPositionChanged().
    if (d->cellTool->editor()) {
        d->cellTool->editor()->setCursorPosition(textCursor().position());
    }
}

void ExternalEditor::slotCursorPositionChanged()
{
    if (!hasFocus() || !d->cellTool->editor()) {
        return;
    }
    // Suppress updates, if this slot got invoked by a text change. It is done
    // later by slotTextChanged().
    if (d->cellTool->editor()->toPlainText() == toPlainText()) {
        d->cellTool->editor()->setCursorPosition(textCursor().position());
    }
}

QAction *ExternalEditor::applyAction() const
{
    return d->applyAction;
}

QAction *ExternalEditor::cancelAction() const
{
    return d->cancelAction;
}
