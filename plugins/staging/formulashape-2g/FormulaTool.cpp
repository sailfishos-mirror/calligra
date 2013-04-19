/* This file is part of the KDE project
 *
 * Copyright (C) 2013 Gopalakrishna Bhat A <gopalakbhat@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// Own
#include "FormulaTool.h"

// Qt
#include <QToolButton>
#include <QGridLayout>
#include <QTextEdit>

// KDE
#include <KLocale>
#include <KUrl>
#include <KFileDialog>
#include <KIO/Job>

// Calligra
#include <KoCanvasBase.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <KoPointerEvent.h>
#include <KoIcon.h>
#include <kundo2command.h>

// This shape
#include "FormulaShape.h"
#include "FormulaCursor.h"

FormulaTool::FormulaTool( KoCanvasBase* canvas )
    : KoToolBase(canvas)
    , m_formulaShape(0)
    , m_textEdit(0)
{
}

void FormulaTool::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    Q_UNUSED(toolActivation);

    foreach (KoShape *shape, shapes) {
        m_formulaShape = dynamic_cast<FormulaShape*>( shape );
        if (m_formulaShape) {
            break;
        }
    }

    if (!m_formulaShape) {
        emit done();
        return;
    }

    m_formulaShape->cursor()->activate();
    m_formulaShape->update();
}

void FormulaTool::deactivate()
{
    m_formulaShape->cursor()->deactivate();
    m_formulaShape = 0;
}

QWidget * FormulaTool::createOptionWidget()
{
    QWidget *optionWidget = new QWidget();
    QGridLayout *layout = new QGridLayout(optionWidget);

    QToolButton *button = 0;

    m_textEdit = new QTextEdit(optionWidget);
    m_textEdit->setText(m_formulaShape->MML());
    button = new QToolButton(optionWidget);
    //button->setIcon(koIcon("document-open"));
    button->setText(i18n("Insert"));
    button->setToolTip(i18n( "Insert"));
    layout->addWidget(button, 1, 0);
    layout->addWidget(m_textEdit, 0, 0);
    connect(button, SIGNAL(clicked(bool)), this, SLOT(insertPressed()));
    connect(this, SIGNAL(formulaContentChanged(const QString &)), m_textEdit, SLOT(setPlainText(const QString &)));
    return optionWidget;
}

void FormulaTool::insertPressed()
{
    if (!m_formulaShape) {
        return;
    }

    m_formulaShape->setMML(m_textEdit->toPlainText());
}

void FormulaTool::mousePressEvent(KoPointerEvent *event)
{
    if (event->button() == Qt::RightButton) {
        event->ignore();
    }
}

void FormulaTool::mouseDoubleClickEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void FormulaTool::paint(QPainter &painter, const KoViewConverter &converter)
{
    Q_UNUSED(converter);
    if (!m_formulaShape) {
        return;
    }

    m_formulaShape->update();
}

void FormulaTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace) {
        if(cursor()) {
            KUndo2Command *command = cursor()->deleteText();
            if (command!=0) {
                canvas()->addCommand(command);
                emit formulaContentChanged(m_formulaShape->MML());
            }
        }
    } else if (event->key() == Qt::Key_Delete) {
        if(cursor()) {
            KUndo2Command *command = cursor()->deleteNode();
            if (command!=0) {
                canvas()->addCommand(command);
                emit formulaContentChanged(m_formulaShape->MML());
            }
        }
    } else if ((event->key() == Qt::Key_Left) && (event->modifiers() & Qt::ControlModifier) == 0) {
        if(cursor()) {
            cursor()->previousNode();
        }
    } else if ((event->key() == Qt::Key_Right) && (event->modifiers() & Qt::ControlModifier) == 0) {
        if(cursor()) {
            cursor()->nextNode();
        }
    } else if ((event->key() == Qt::Key_Up) && (event->modifiers() & Qt::ControlModifier) == 0) {

    } else if ((event->key() == Qt::Key_Down) && (event->modifiers() & Qt::ControlModifier) == 0) {

    } else {
        if(cursor()) {
            KUndo2Command *command = cursor()->insertText(event->text());
            if (command!=0) {
                canvas()->addCommand(command);
                emit formulaContentChanged(m_formulaShape->MML());
            }
        }
    }
}

FormulaCursor *FormulaTool::cursor()
{
    if (!m_formulaShape) {
        return 0;
    }

    return m_formulaShape->cursor();
}

#include <FormulaTool.moc>
