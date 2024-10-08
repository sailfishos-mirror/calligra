/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Martin Pfeiffer <hubipete@gmx.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FORMULATOOLWIDGET_H
#define FORMULATOOLWIDGET_H

#include "ui_FormulaToolWidget.h"
#include <QMenu>
#include <QTabWidget>

class KoFormulaTool;
class QTableWidgetItem;

/**
 * @short A widget providing options for the FormulaTool
 *
 * The FormulaToolOptions widget provides the combobox - listwidget combination
 * which is used to select templates for inserting in the formula. Further the
 * widget provides two buttons - save and load formula. For saving and loading it
 * uses the loadOdf() and saveOdf() methods of KoFormulaShape.
 */
class FormulaToolWidget : public QWidget, Ui::FormulaToolWidget
{
    Q_OBJECT
public:
    /// Standard constructor
    explicit FormulaToolWidget(KoFormulaTool *tool, QWidget *parent = nullptr);

    /// Standard destructor
    ~FormulaToolWidget() override;

    /// Set the KoFormulaTool @p tool this options widget belongs to
    void setFormulaTool(KoFormulaTool *tool);

public Q_SLOTS:
    void insertSymbol(QTableWidgetItem *item);

private:
    void setupButton(QToolButton *button, QMenu &menu, const QString &text, QList<QString>, int length = 8);

    static QList<QString> symbolsInRange(int start, int length);

private:
    /// The KoFormulaTool this options widget belongs to
    KoFormulaTool *m_tool;

    QMenu m_scriptsMenu;
    QMenu m_fractionMenu;
    QMenu m_tableMenu;
    QMenu m_fenceMenu;
    QMenu m_rootMenu;
    QMenu m_arrowMenu;
    QMenu m_greekMenu;
    QMenu m_miscMenu;
    QMenu m_relationMenu;
    QMenu m_operatorMenu;
    QMenu m_alterTableMenu;
    QMenu m_rowMenu;
};

#endif // FORMULATOOLWIDGET_H
