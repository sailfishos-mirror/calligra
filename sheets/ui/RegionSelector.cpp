/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1999-2006 The KSpread Team <calligra-devel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "RegionSelector.h"

// Sheets
#include "FormulaEditorHighlighter.h"
#include "Selection.h"

// Calligra
#include <KoDialog.h>
#include <KoIcon.h>

// KF5
#include <KLocalizedString>
#include <ktextedit.h>

// Qt
#include <QHBoxLayout>
#include <QToolButton>

using namespace Calligra::Sheets;

class Q_DECL_HIDDEN RegionSelector::Private
{
public:
    Selection *selection;
    QDialog *parentDialog;
    KoDialog *dialog;
    KTextEdit *textEdit;
    QToolButton *button;
    FormulaEditorHighlighter *highlighter;
    DisplayMode displayMode;
    SelectionMode selectionMode;
    static RegionSelector *s_focussedSelector;
};

RegionSelector *RegionSelector::Private::s_focussedSelector = nullptr;

RegionSelector::RegionSelector(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    d->displayMode = Widget;
    d->parentDialog = nullptr;
    d->selection = nullptr;
    d->dialog = nullptr;
    d->button = new QToolButton(this);
    d->button->setCheckable(true);
    d->button->setIcon(koIcon("selection"));
    d->highlighter = nullptr;
    d->textEdit = new KTextEdit(this);
    d->textEdit->setLineWrapMode(QTextEdit::NoWrap);
    d->textEdit->setWordWrapMode(QTextOption::NoWrap);
    d->textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    d->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->textEdit->setFixedHeight(d->button->height() - 2 * d->textEdit->frameWidth()); // FIXME
    d->textEdit->setTabChangesFocus(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(2);
    layout->addWidget(d->textEdit);
    layout->addWidget(d->button);

    d->button->installEventFilter(this);
    d->textEdit->installEventFilter(this);
    connect(d->button, &QAbstractButton::toggled, this, &RegionSelector::switchDisplayMode);
}

RegionSelector::~RegionSelector()
{
    d->selection->endReferenceSelection();
    d->selection->setSelectionMode(Selection::MultipleCells);
    delete d;
}

void RegionSelector::setSelectionMode(SelectionMode mode)
{
    d->selectionMode = mode;
    // TODO adjust selection
}

void RegionSelector::setSelection(Selection *selection)
{
    d->selection = selection;
    d->highlighter = new FormulaEditorHighlighter(d->textEdit, d->selection);
    connect(d->selection, &Selection::changed, this, &RegionSelector::choiceChanged);
}

void RegionSelector::setDialog(QDialog *dialog)
{
    d->parentDialog = dialog;
}

KTextEdit *RegionSelector::textEdit() const
{
    return d->textEdit;
}

bool RegionSelector::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        if (object == d->dialog && d->button->isChecked()) {
            // TODO Stefan: handle as button click
            //       d->button->toggle();
            event->ignore();
            return true; // eat it
        }
    } else if (event->type() == QEvent::FocusIn) {
        Private::s_focussedSelector = this;
        d->selection->startReferenceSelection();
        if (d->selectionMode == SingleCell) {
            d->selection->setSelectionMode(Selection::SingleCell);
        } else {
            d->selection->setSelectionMode(Selection::MultipleCells);
        }
        // TODO Stefan: initialize choice
    }
    return QObject::eventFilter(object, event);
}

void RegionSelector::switchDisplayMode(bool state)
{
    Q_UNUSED(state)
    debugSheets;

    if (d->displayMode == Widget) {
        d->displayMode = Dialog;

        d->dialog = new KoDialog(d->parentDialog->parentWidget(), Qt::Tool);
        d->dialog->resize(d->parentDialog->width(), 20);
        d->dialog->move(d->parentDialog->pos());
        d->dialog->setButtons({});
        d->dialog->setModal(false);

        if (d->selectionMode == SingleCell) {
            d->dialog->setCaption(i18n("Select Single Cell"));
        } else { // if ( d->selectionMode == MultipleCells )
            d->dialog->setCaption(i18n("Select Multiple Cells"));
        }

        QWidget *widget = new QWidget(d->dialog);
        QHBoxLayout *layout = new QHBoxLayout(widget);
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->textEdit);
        layout->addWidget(d->button);

        d->dialog->setMainWidget(widget);
        d->dialog->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        d->dialog->installEventFilter(this);
        d->dialog->show();
        d->parentDialog->hide();
    } else {
        d->displayMode = Widget;

        layout()->addWidget(d->textEdit);
        layout()->addWidget(d->button);

        d->parentDialog->move(d->dialog->pos());
        d->parentDialog->show();
        delete d->dialog;
        d->dialog = nullptr;
    }
}

void RegionSelector::choiceChanged()
{
    if (Private::s_focussedSelector != this)
        return;

    if (d->selection->isValid()) {
        QString area = d->selection->name();
        d->textEdit->setPlainText(area);
    }
}
