/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1999-2006 The KSpread Team <calligra-devel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "FunctionCompletion.h"

// Sheets
#include "CellEditor.h"
#include "FunctionDescription.h"
#include "FunctionRepository.h"

// Qt
#include <QApplication>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QScrollBar>
#include <QToolTip>
#include <QVBoxLayout>

using namespace Calligra::Sheets;

class FunctionCompletion::Private
{
public:
    CellEditor *editor;
    QFrame *completionPopup;
    QListWidget *completionListBox;
    QLabel *hintLabel;
};

FunctionCompletion::FunctionCompletion(CellEditor *editor)
    : QObject(editor)
    , d(new Private)
{
    d->editor = editor;
    d->hintLabel = nullptr;

    d->completionPopup = new QFrame(editor->topLevelWidget(), Qt::Popup);
    d->completionPopup->setFrameStyle(QFrame::Box | QFrame::Plain);
    d->completionPopup->setLineWidth(1);
    d->completionPopup->installEventFilter(this);
    d->completionPopup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout *layout = new QVBoxLayout(d->completionPopup);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    d->completionListBox = new QListWidget(d->completionPopup);
    d->completionPopup->setFocusProxy(d->completionListBox);
    d->completionListBox->setFrameStyle(QFrame::NoFrame);
    //   d->completionListBox->setVariableWidth( true );
    d->completionListBox->installEventFilter(this);
    connect(d->completionListBox, &QListWidget::currentRowChanged, this, [this]() {
        itemSelected();
    });
    // When items are activated on single click, also change the help page on mouse-over, otherwise there is no (easy) way to get
    // the help (with the mouse) without inserting the function
    if (d->completionListBox->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, d->completionListBox)) {
        connect(d->completionListBox, &QListWidget::itemEntered, this, &FunctionCompletion::itemSelected);
        d->completionListBox->setMouseTracking(true);
    }

    connect(d->completionListBox, &QListWidget::itemActivated, this, &FunctionCompletion::doneCompletion);
    layout->addWidget(d->completionListBox);

    d->hintLabel = new QLabel(nullptr, Qt::FramelessWindowHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
    d->hintLabel->setFrameStyle(QFrame::Plain | QFrame::Box);
    d->hintLabel->setPalette(QToolTip::palette());
    d->hintLabel->setWordWrap(true);
    d->hintLabel->hide();
}

FunctionCompletion::~FunctionCompletion()
{
    delete d->hintLabel;
    delete d;
}

void FunctionCompletion::itemSelected(QListWidgetItem *listItem)
{
    QString item;
    if (listItem) {
        item = listItem->text();
    } else {
        listItem = d->completionListBox->currentItem();
        if (listItem) {
            item = listItem->text();
        }
    }

    Calligra::Sheets::FunctionDescription *desc;
    desc = Calligra::Sheets::FunctionRepository::self()->functionInfo(item);
    if (!desc) {
        d->hintLabel->hide();
        return;
    }

    const QStringList helpTexts = desc->helpText();
    QString helpText = helpTexts.isEmpty() ? QString() : helpTexts.first();
    if (helpText.isEmpty()) {
        d->hintLabel->hide();
        return;
    }

    helpText.append("</qt>").prepend("<qt>");
    d->hintLabel->setText(helpText);
    d->hintLabel->adjustSize();

    // reposition nicely
    QPoint pos = d->editor->mapToGlobal(QPoint(d->editor->width(), 0));
    pos.setY(pos.y() - d->hintLabel->height() - 1);
    d->hintLabel->move(pos);
    d->hintLabel->show();
    d->hintLabel->raise();

    // do not show it forever
    // QTimer::singleShot( 5000, d->hintLabel, SLOT(hide()) );
}

bool FunctionCompletion::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != d->completionPopup && obj != d->completionListBox)
        return false;

    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        switch (ke->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            doneCompletion();
            return true;
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            return false;
        default:
            d->hintLabel->hide();
            d->completionPopup->close();
            d->editor->setFocus();
            QApplication::sendEvent(d->editor, ev);
            return true;
        }
    }

    if (ev->type() == QEvent::Close) {
        d->hintLabel->hide();
    }

    if (ev->type() == QEvent::MouseButtonDblClick) {
        doneCompletion();
        return true;
    }
    return false;
}

void FunctionCompletion::doneCompletion()
{
    d->hintLabel->hide();
    d->completionPopup->close();
    d->editor->setFocus();
    Q_EMIT selectedCompletion(d->completionListBox->currentItem()->text());
}

void FunctionCompletion::showCompletion(const QStringList &choices)
{
    if (!choices.count())
        return;

    d->completionListBox->clear();
    d->completionListBox->addItems(choices);
    d->completionListBox->setCurrentItem(nullptr);

    // size of the pop-up
    d->completionPopup->setMaximumHeight(100);
    d->completionPopup->resize(d->completionListBox->sizeHint()
                               + QSize(d->completionListBox->verticalScrollBar()->width() + 4, d->completionListBox->horizontalScrollBar()->height() + 4));
    QPoint pos = d->editor->globalCursorPosition();

    // if popup is partially invisible, move to other position
    // FIXME check it if it works in Xinerama multihead
    // POrt to wayland
    //
    // int h = d->completionListBox->height();
    // int w = d->completionListBox->width();
    // int screen_num = QApplication::desktop()->screenNumber(d->completionPopup);
    // QRect screen = QApplication::desktop()->screenGeometry(screen_num);
    // if (pos.y() + h > screen.y() + screen.height())
    //     pos.setY(pos.y() - h - d->editor->height());
    // if (pos.x() + w > screen.x() + screen.width())
    //     pos.setX(screen.x() + screen.width() - w);

    d->completionPopup->move(pos);
    d->completionListBox->setFocus();
    d->completionPopup->show();
}
