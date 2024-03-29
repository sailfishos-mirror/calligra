// This file is part of KSpread
// SPDX-FileCopyrightText: 2007 Sebastian Sauer <mail@dipe.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "ScriptingWidgets.h"

#include "ScriptingModule.h"
#include "ScriptingDebug.h"

#include <QMetaEnum>
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>

#include <KLocalizedString>

#include <engine/Region.h>
#include <core/Map.h>
#include <core/Sheet.h>
#include <part/Doc.h>
#include <part/View.h>

ScriptingSheetsListView::ScriptingSheetsListView(ScriptingModule* module, QWidget* parent)
        : QWidget(parent), m_module(module), m_initialized(false), m_selectiontype(SingleSelect), m_editortype(Disabled)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);
    m_view = new QTreeView(this);
    //m_view->setAlternatingRowColors(true);
    m_view->setRootIsDecorated(false);
    m_view->setSortingEnabled(false);
    m_view->setItemsExpandable(false);
    m_view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_view->setModel(new QStandardItemModel(this));
    layout->addWidget(m_view);
}

ScriptingSheetsListView::~ScriptingSheetsListView()
{
}

void ScriptingSheetsListView::setSelectionType(const QString& selectiontype)
{
    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("SelectionType"));
    int v = e.keysToValue(selectiontype.toUtf8());
    if (v >= 0) m_selectiontype = (SelectionType) v;
}

void ScriptingSheetsListView::setEditorType(const QString& editortype)
{
    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("EditorType"));
    int v = e.keysToValue(editortype.toUtf8());
    if (v >= 0) m_editortype = (EditorType) v;
}

void ScriptingSheetsListView::initialize()
{
    if (m_initialized)
        finalize();

    debugSheetsScripting << "ScriptingSheetsListView::initialize()";

    QStringList headers;
    headers << i18n("Sheet");
    switch (m_editortype) {
    case Disabled: break;
    case Cell: headers << i18n("Cell"); break;
    case Range: headers << i18n("Range"); break;
    }

    QStandardItemModel* model = static_cast< QStandardItemModel* >(m_view->model());
    model->setHorizontalHeaderLabels(headers);
    Calligra::Sheets::Doc* doc = m_module->kspreadDoc();
    Calligra::Sheets::View* view = m_module->kspreadView();
    Calligra::Sheets::Sheet* activeSheet = view ? view->activeSheet() : 0;
    if (doc && doc->map()) {
        for(Calligra::Sheets::SheetBase* bsheet : doc->map()->sheetList()) {
            Calligra::Sheets::Sheet *sheet = dynamic_cast<Calligra::Sheets::Sheet *>(bsheet);
            if (! sheet || sheet->isHidden())
                continue;
            QRect area = sheet->usedArea();
            bool enabled = area.isValid();
            Q_UNUSED(enabled);
            QList< QStandardItem* > items;

            QStandardItem* nameitem = new QStandardItem(sheet->sheetName());
            nameitem->setEditable(false);
            if (m_selectiontype == MultiSelect) {
                nameitem->setCheckable(true);
                nameitem->setCheckState((activeSheet == sheet) ? Qt::Checked : Qt::Unchecked);
            }
            items << nameitem;

            if (m_editortype != Disabled) {
                QString range;
                for(const QVariant &v : m_prevlist) {
                    QVariantList l = v.toList();
                    if (l.count() < 1 || l[0].toString() != sheet->sheetName())
                        continue;
                    if (l.count() >= 2)
                        if (m_selectiontype == MultiSelect)
                            nameitem->setCheckState(l[1].toBool() ? Qt::Checked : Qt::Unchecked);
                    if (l.count() >= 3) {
                        QStringList rangelist;
                        for (int i = 2; i < l.count(); ++i) {
                            const QRect rect = l[i].toRect();
                            if (rect.isNull())
                                continue;
                            Calligra::Sheets::Region region(rect, sheet);
                            for (Calligra::Sheets::Region::ConstIterator it = region.constBegin(); it != region.constEnd(); ++it) {
                                const QString n = (*it)->name(sheet);
                                if (! n.isEmpty())
                                    rangelist.append(n);
                            }
                        }
                        range = rangelist.join(";");
                    }
                    break;
                }
                if (range.isEmpty() && area.isValid())
                    range = Calligra::Sheets::Region(area, sheet).name(sheet);
                if (m_editortype == Cell) {
                    int p = range.indexOf(':');
                    range = p > 0 ? range.left(p) : "A1";
                }
                items << new QStandardItem(range);
            }

            model->appendRow(items);
            if (activeSheet == sheet)
                m_view->setCurrentIndex(nameitem->index());
        }
    }

    m_initialized = true;
}

void ScriptingSheetsListView::finalize()
{
    if (m_initialized) {
        debugSheetsScripting << "ScriptingSheetsListView::finalize()";

        m_prevlist = sheets();
        QStandardItemModel* model = static_cast< QStandardItemModel* >(m_view->model());
        model->clear();
        m_initialized = false;
    }
}

void ScriptingSheetsListView::showEvent(QShowEvent* event)
{
    finalize();
    QWidget::showEvent(event);
    initialize();
}

QString ScriptingSheetsListView::sheet()
{
    if (! m_initialized)
        initialize();
    QStandardItemModel* model = static_cast< QStandardItemModel* >(m_view->model());
    QStandardItem* current = model->itemFromIndex(m_view->currentIndex());
    QStandardItem* nameitem = current ? model->item(current->row(), 0) : 0;
    return nameitem ? nameitem->text() : QString();
}

QString ScriptingSheetsListView::editor()
{
    if (! m_initialized)
        initialize();
    QStandardItemModel* model = static_cast< QStandardItemModel* >(m_view->model());
    QStandardItem* current = model->itemFromIndex(m_view->currentIndex());
    QStandardItem* rangeitem = current ? model->item(current->row(), 1) : 0;
    return rangeitem ? rangeitem->text() : QString();
}

QVariantList ScriptingSheetsListView::sheets()
{
    if (! m_initialized)
        initialize();

    QVariantList list;
    QStandardItemModel* model = static_cast< QStandardItemModel* >(m_view->model());
    const int rowcount = model->rowCount();
    for (int row = 0; row < rowcount; ++row) {
        QStandardItem* nameitem = model->item(row, 0);
        if (! nameitem)
            continue;

        bool enabled = nameitem->checkState() == Qt::Checked;

        const QString sheetname = nameitem->text();
        Calligra::Sheets::SheetBase* sheet = m_module->kspreadDoc()->map()->findSheet(sheetname);
        if (! sheet)
            continue;

        QVariantList l;
        l << sheetname << enabled;

        QStandardItem* rangeitem = model->item(row, 1);
        if (rangeitem) {
            const QString range = rangeitem->text();
            Calligra::Sheets::Region region = m_module->kspreadDoc()->map()->regionFromName(range, sheet);
            for (Calligra::Sheets::Region::ConstIterator it = region.constBegin(); it != region.constEnd(); ++it) {
                const QRect rect = (*it)->rect();
                if (! rect.isNull())
                    l << rect;
            }
        }

        list.append(l);
    }
    return list;
}
