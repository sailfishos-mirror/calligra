/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2018 Dag Andersen <danders@get2net.dk>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ScatterDataEditor.h"
// #include "DataSetTableModel.h"

#include <QAbstractTableModel>
#include <QAction>
#include <QSortFilterProxyModel>

#include <KoIcon.h>

#include "ChartDebug.h"
#include "ChartProxyModel.h"
#include "ChartShape.h"
#include "ChartTableModel.h"
#include "ChartTableView.h"
#include "DataSet.h"

namespace KoChart
{

namespace Scatter
{

class DataProxy : public QSortFilterProxyModel
{
public:
    DataSetTableModel *dataSetModel;

    DataProxy(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , dataSetModel(nullptr)
    {
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                return CellRegion::columnName(section + 2);
            } else {
                return section + 1;
            }
        }
        return QSortFilterProxyModel::headerData(section, orientation, role);
    }
    bool filterAcceptsColumn(int source_column, const QModelIndex & /*source_parent*/) const override
    {
        return source_column != 0; // skip categories
    }
    bool insertColumns(int column, int count, const QModelIndex &parent) override
    {
        debugChartUiScatter << column;
        Q_UNUSED(count);
        Q_UNUSED(parent);

        QAbstractItemModel *model = sourceModel();
        int scolumn = mapToSource(index(0, column)).column();
        if (scolumn < 0) {
            scolumn = model->columnCount();
        }
        debugChartUiScatter << column << ':' << scolumn;
        if (!model->insertColumns(scolumn, 1)) {
            return false;
        }
        // Label
        for (int r = 0; r < model->rowCount(); ++r) {
            if (r == 0) {
                QModelIndex idx = model->index(0, scolumn);
                model->setData(idx, i18n("Column %1", scolumn));
            } else {
                QModelIndex idx = model->index(r, scolumn);
                model->setData(idx, (double)r);
            }
        }
        return true;
    }
    bool insertRows(int row, int count, const QModelIndex &parent) override
    {
        debugChartUiScatter << row;
        Q_UNUSED(count);
        Q_UNUSED(parent);

        QAbstractItemModel *model = sourceModel();
        int srow = mapToSource(index(row, 0)).row();
        if (srow < 0) {
            srow = model->rowCount();
        }
        if (!model->insertRows(srow, 1)) {
            return false;
        }
        QModelIndex idx = model->index(srow, 0);
        model->setData(idx, i18n("Row %1", srow + 1));
        for (int c = 1; c < model->columnCount(); ++c) {
            QModelIndex idx = model->index(srow, c);
            model->setData(idx, (double)c);
        }
        return true;
    }
};

} // namespace Scatter

using namespace Scatter;

ScatterDataEditor::ScatterDataEditor(ChartShape *chart, QWidget *parent)
    : KoDialog(parent)
    , m_chart(chart)
{
    setCaption(i18n("Scatter Data Editor"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    QWidget *w = new QWidget(this);
    m_ui.setupUi(w);
    setMainWidget(w);

    m_insertColumnBeforeAction = new QAction(m_ui.insertColumnBefore->icon(), i18n("Insert Column Before"), m_ui.tableView);
    m_insertColumnAfterAction = new QAction(m_ui.insertColumnAfter->icon(), i18n("Insert Column After"), m_ui.tableView);
    m_insertRowAboveAction = new QAction(m_ui.insertRowAbove->icon(), i18n("Insert Row Above"), m_ui.tableView);
    m_insertRowBelowAction = new QAction(m_ui.insertRowBelow->icon(), i18n("Insert Row Below"), m_ui.tableView);
    m_deleteAction = new QAction(m_ui.deleteSelection->icon(), i18n("Delete"), m_ui.tableView);

    m_ui.tableView->addAction(m_insertColumnBeforeAction);
    m_ui.tableView->addAction(m_insertColumnAfterAction);
    m_ui.tableView->addAction(m_insertRowAboveAction);
    m_ui.tableView->addAction(m_insertRowBelowAction);
    m_ui.tableView->addAction(m_deleteAction);

    m_ui.tableView->setContextMenuPolicy(Qt::ActionsContextMenu);

    m_ui.deleteSelection->setEnabled(false);
    m_deleteAction->setEnabled(false);

    m_dataSetModel.tableSource = m_chart->tableSource();
    connect(m_ui.addDataSetBefore, &QAbstractButton::clicked, this, &ScatterDataEditor::slotAddDataSetBefore);
    connect(m_ui.addDataSetAfter, &QAbstractButton::clicked, this, &ScatterDataEditor::slotAddDataSetAfter);
    connect(m_ui.removeDataSet, &QAbstractButton::clicked, this, &ScatterDataEditor::slotRemoveDataSet);

    m_dataSetModel.setModel(m_chart->proxyModel());
    //     connect(&m_dataSetModel, &Scatter::DataSetTableModel::dataChanged, this, &ScatterDataEditor::slotDataChanged);

    m_dataModel = new DataProxy(m_ui.tableView);
    m_dataModel->setSourceModel(m_chart->internalModel());
    m_dataModel->dataSetModel = &m_dataSetModel;

    connect(m_ui.insertColumnBefore, &QAbstractButton::clicked, this, &ScatterDataEditor::slotInsertColumnBefore);
    connect(m_ui.insertColumnAfter, &QAbstractButton::clicked, this, &ScatterDataEditor::slotInsertColumnAfter);
    connect(m_ui.insertRowAbove, &QAbstractButton::clicked, this, &ScatterDataEditor::slotInsertRowAbove);
    connect(m_ui.insertRowBelow, &QAbstractButton::clicked, this, &ScatterDataEditor::slotInsertRowBelow);
    connect(m_ui.deleteSelection, &QAbstractButton::clicked, this, &ScatterDataEditor::slotDeleteSelection);

    connect(m_insertColumnBeforeAction, &QAction::triggered, this, &ScatterDataEditor::slotInsertColumnBefore);
    connect(m_insertColumnAfterAction, &QAction::triggered, this, &ScatterDataEditor::slotInsertColumnAfter);
    connect(m_insertRowAboveAction, &QAction::triggered, this, &ScatterDataEditor::slotInsertRowAbove);
    connect(m_insertRowBelowAction, &QAction::triggered, this, &ScatterDataEditor::slotInsertRowBelow);
    connect(m_deleteAction, &QAction::triggered, this, &ScatterDataEditor::slotDeleteSelection);

    connect(m_ui.tableView, &ChartTableView::currentIndexChanged, this, &ScatterDataEditor::enableActions);

    m_ui.tableView->setModel(m_dataModel);
    m_ui.dataSetView->setModel(&m_dataSetModel);

    DataColumnDelegate *delegate = new DataColumnDelegate(m_ui.dataSetView);
    delegate->dataModel = m_dataModel;
    m_ui.dataSetView->setItemDelegateForColumn(0, delegate);
    m_ui.dataSetView->setItemDelegateForColumn(1, delegate);
    m_ui.dataSetView->setItemDelegateForColumn(2, delegate);
    m_ui.dataSetView->setItemDelegateForColumn(3, delegate);

    m_ui.dataSetView->verticalHeader()->hide();

    connect(m_ui.manualControl, &QAbstractButton::toggled, m_chart->proxyModel(), &ChartProxyModel::setManualControl);

    connect(m_ui.tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ScatterDataEditor::enableActions);
    connect(m_ui.dataSetView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ScatterDataEditor::enableActions);
    connect(m_ui.manualControl, &QAbstractButton::clicked, this, &ScatterDataEditor::enableActions);

    chart->proxyModel()->setManualControl(m_ui.manualControl->isChecked());
    enableActions();

    connect(m_dataModel, &QAbstractItemModel::columnsInserted, this, &ScatterDataEditor::dataColumnsInserted);
    connect(m_dataModel, &QAbstractItemModel::columnsRemoved, this, &ScatterDataEditor::dataColumnsRemoved);
    connect(m_dataModel->sourceModel(), &QAbstractItemModel::rowsInserted, this, &ScatterDataEditor::dataRowCountChanged);
    connect(m_dataModel->sourceModel(), &QAbstractItemModel::rowsRemoved, this, &ScatterDataEditor::dataRowCountChanged);

    resize(sizeHint().expandedTo(QSize(600, 300)));
}

ScatterDataEditor::~ScatterDataEditor() = default;

void ScatterDataEditor::slotInsertColumnBefore()
{
    debugChartUiScatter << m_ui.tableView->currentIndex();
    int pos = m_ui.tableView->currentIndex().column();
    if (pos < 0) {
        pos = 0;
    }
    m_dataModel->insertColumn(pos);
}

void ScatterDataEditor::slotInsertColumnAfter()
{
    debugChartUiScatter << m_ui.tableView->currentIndex();
    int pos = m_ui.tableView->currentIndex().column() + 1;
    if (pos == 0) {
        pos = m_dataModel->columnCount();
    }
    m_dataModel->insertColumn(pos);
}

void ScatterDataEditor::slotInsertRowAbove()
{
    debugChartUiScatter << m_ui.tableView->currentIndex();
    int pos = m_ui.tableView->currentIndex().row();
    if (pos < 0) {
        pos = 0;
    }
    m_dataModel->insertRow(pos);
}

void ScatterDataEditor::slotInsertRowBelow()
{
    debugChartUiScatter << m_ui.tableView->currentIndex();
    int pos = m_ui.tableView->currentIndex().row() + 1;
    if (pos == 0) {
        pos = m_dataModel->rowCount();
    }
    m_dataModel->insertRow(pos);
}

void ScatterDataEditor::slotDeleteSelection()
{
    QAbstractItemModel *model = m_ui.tableView->model();
    QModelIndexList lst = m_ui.tableView->selectionModel()->selectedRows();
    if (!lst.isEmpty()) {
        for (int i = lst.count() - 1; i >= 0; --i) {
            model->removeRow(lst.at(i).row());
        }
    } else {
        QModelIndexList lst = m_ui.tableView->selectionModel()->selectedColumns();
        for (int i = lst.count() - 1; i >= 0; --i) {
            model->removeColumn(lst.at(i).column());
        }
    }
}

void ScatterDataEditor::slotAddDataSetBefore()
{
    debugChartUiScatter << m_ui.tableView->currentIndex();
    int pos = m_ui.dataSetView->currentIndex().row();
    if (pos < 0) {
        pos = 0;
    }
    m_dataSetModel.model()->insertRow(pos);
}

void ScatterDataEditor::slotAddDataSetAfter()
{
    debugChartUiScatter << m_ui.dataSetView->currentIndex();
    int pos = m_ui.dataSetView->currentIndex().row() + 1;
    if (pos == 0) {
        pos = m_dataSetModel.rowCount();
    }
    m_dataSetModel.model()->insertRow(pos);
}

void ScatterDataEditor::slotRemoveDataSet()
{
    int row = m_ui.dataSetView->selectionModel()->currentIndex().row();
    if (row > 0) {
        debugChartUiScatter << row << m_ui.dataSetView->model();
        m_ui.dataSetView->model()->removeRow(row);
    }
}

void ScatterDataEditor::enableActions()
{
    QItemSelectionModel *smodel = m_ui.tableView->selectionModel();
    m_ui.insertRowAbove->setEnabled(smodel && smodel->currentIndex().row() > 0);
    m_ui.deleteSelection->setEnabled(smodel && (!smodel->selectedRows().isEmpty() || !smodel->selectedColumns().isEmpty()));
    m_deleteAction->setEnabled(m_ui.deleteSelection->isEnabled());

    smodel = m_ui.dataSetView->selectionModel();
    m_ui.addDataSetBefore->setEnabled(m_ui.manualControl->isChecked());
    m_ui.addDataSetAfter->setEnabled(m_ui.manualControl->isChecked());
    m_ui.removeDataSet->setEnabled(m_ui.manualControl->isChecked() && smodel && smodel->currentIndex().isValid());
}

void ScatterDataEditor::dataColumnsInserted(const QModelIndex &, int first, int last)
{
    Q_ASSERT(first == last);
    if (!m_ui.manualControl->isChecked() || first == m_dataModel->columnCount() - 1) {
        return;
    }
    first += 2; // column 0 is hidden and rects starts at 1 (A=1, B=2...)
    for (DataSet *ds : m_chart->proxyModel()->dataSets()) {
        CellRegion region = ds->xDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                ds->setXDataRegion(CellRegion(region.table(), r.adjusted(1, 0, 1, 0)));
                debugChartUiScatter << "move X:" << first << ':' << r << region.toString() << ':' << ds->xDataRegion().toString();
                break;
            }
        }
        region = ds->yDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                ds->setYDataRegion(CellRegion(region.table(), r.adjusted(1, 0, 1, 0)));
                debugChartUiScatter << "move Y:" << first << ':' << r << region.toString() << ':' << ds->xDataRegion().toString();
                break;
            }
        }
        region = ds->customDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                debugChartUiScatter << "move Cust:" << first << ':' << r;
                ds->setCustomDataRegion(CellRegion(region.table(), r.adjusted(1, 0, 1, 0)));
                break;
            }
        }
        region = ds->categoryDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                debugChartUiScatter << "move Cat:" << first << ':' << r;
                ds->setCategoryDataRegion(CellRegion(region.table(), r.adjusted(1, 0, 1, 0)));
                break;
            }
        }
        region = ds->labelDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                debugChartUiScatter << "move Lab:" << first << ':' << r;
                ds->setLabelDataRegion(CellRegion(region.table(), r.adjusted(1, 0, 1, 0)));
                break;
            }
        }
    }
}

void ScatterDataEditor::dataColumnsRemoved(const QModelIndex &, int first, int last)
{
    if (!m_ui.manualControl->isChecked()) {
        return;
    }
    int count = last - first + 1;
    first += 2; // column 0 is hidden and rects starts at 1 (A=1, B=2...)
    for (DataSet *ds : m_chart->proxyModel()->dataSets()) {
        CellRegion region = ds->xDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                ds->setXDataRegion(CellRegion(region.table(), r.adjusted(-count, 0, -count, 0)));
                debugChartUiScatter << "move X:" << first << ':' << r << region.toString() << ':' << ds->xDataRegion().toString();
                break;
            }
        }
        region = ds->yDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                ds->setYDataRegion(CellRegion(region.table(), r.adjusted(-count, 0, -count, 0)));
                debugChartUiScatter << "move Y:" << first << ':' << r << region.toString() << ':' << ds->xDataRegion().toString();
                break;
            }
        }
        region = ds->customDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                debugChartUiScatter << "move Cust:" << first << ':' << r;
                ds->setCustomDataRegion(CellRegion(region.table(), r.adjusted(-count, 0, -count, 0)));
                break;
            }
        }
        region = ds->categoryDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                debugChartUiScatter << "move Cat:" << first << ':' << r;
                ds->setCategoryDataRegion(CellRegion(region.table(), r.adjusted(-count, 0, -count, 0)));
                break;
            }
        }
        region = ds->labelDataRegion();
        for (QRect r : region.rects()) {
            if (r.left() >= first) {
                debugChartUiScatter << "move Lab:" << first << ':' << r;
                ds->setLabelDataRegion(CellRegion(region.table(), r.adjusted(-count, 0, -count, 0)));
                break;
            }
        }
    }
}

void ScatterDataEditor::dataRowCountChanged()
{
    if (!m_chart->proxyModel()->manualControl()) {
        debugChartUiScatter << "Not manual control";
        return;
    }
    const QList<DataSet *> lst = m_chart->proxyModel()->dataSets();
    for (int i = 0; i < lst.count(); ++i) {
        DataSet *ds = lst.at(i);
        CellRegion region = ds->xDataRegion();
        if (!region.rects().isEmpty()) {
            QRect r = region.rects().first();
            r.setHeight(m_dataModel->rowCount());
            ds->setXDataRegion(CellRegion(region.table(), r));
        }
        region = ds->yDataRegion();
        if (!region.rects().isEmpty()) {
            QRect r = region.rects().first();
            r.setHeight(m_dataModel->rowCount());
            ds->setYDataRegion(CellRegion(region.table(), r));
        }
        region = ds->customDataRegion();
        if (!region.rects().isEmpty()) {
            QRect r = region.rects().first();
            r.setHeight(m_dataModel->rowCount());
            ds->setCustomDataRegion(CellRegion(region.table(), r));
        }
        region = ds->categoryDataRegion();
        if (!region.rects().isEmpty()) {
            QRect r = region.rects().first();
            r.setHeight(m_dataModel->rowCount());
            ds->setCategoryDataRegion(CellRegion(region.table(), r));
        }
        debugChartUiScatter << ds;
    }
}

} // namespace KoChart
