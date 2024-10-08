/*
 *  SPDX-FileCopyrightText: 2006 Thorsten Zachmann <zachmann@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "rtreetestapp.h"

#include <QApplication>
#include <QDebug>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QStatusBar>
#include <QTimer>

#include "Tool.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mainWin;

    mainWin.show();
    return app.exec();
}

Canvas::Canvas()
    : QWidget()
    , m_zoom(1)
    , m_rtree(4, 2)
    //, m_rtree( 2, 1 )
    , m_tool(nullptr)
    , m_createTool(this)
    , m_selectTool(this)
    , m_removeTool(this)
    , m_file("data.trc")
    , m_listId(0)
    , m_paintTree(false)
{
    m_tool = &m_createTool;
    setBackgroundRole(QPalette::Base);
    m_file.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
    m_out.setDevice(&m_file);
}

void Canvas::updateCanvas()
{
    update();
}

void Canvas::insert(QRectF &rect)
{
    m_out << "i " << rect.left() << " " << rect.top() << " " << rect.width() << " " << rect.height() << Qt::endl;
    Data *data = new Data(rect);
    m_rects.insert(data);
    m_rtree.insert(rect, data);
    update();
}

void Canvas::select(QRectF &rect)
{
    if (rect.isEmpty()) {
        m_found = m_rtree.contains(rect.topLeft());
    } else {
        m_found = m_rtree.intersects(rect);
    }
    update();
}

void Canvas::remove(QRectF &rect)
{
    m_out << "r " << rect.left() << " " << rect.top() << " " << rect.width() << " " << rect.height() << Qt::endl;
    m_found = QList<Data *>();
    QList<Data *> remove = m_rtree.intersects(rect);
    foreach (Data *data, remove) {
        m_rtree.remove(data);
        m_rects.remove(data);
        delete data;
    }
    update();
}

void Canvas::clear()
{
    m_out << "c" << Qt::endl;
    m_rtree.clear();
    qDeleteAll(m_rects);
    m_rects.clear();
    update();
}

void Canvas::replay()
{
    if (QCoreApplication::arguments().size() > 1) {
        QString filename(QCoreApplication::arguments().at(1));
        qDebug() << "parameter:" << filename;
        QFile file(filename);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&file);
        while (!in.atEnd()) {
            m_list.push_back(in.readLine());
        }
        qDebug() << "commands:" << m_list.size();
        m_listId = 0;
        QTimer::singleShot(1000, this, &Canvas::replayStep);
    }
}

void Canvas::replayStep()
{
    QString line = m_list.at(m_listId++);
    qDebug() << "Line:" << line;
    QStringList values = line.split(' ');
    if (values[0] == "c") {
        clear();
    } else {
        int left = values[1].toInt();
        int top = values[2].toInt();
        int right = values[3].toInt();
        int bottom = values[4].toInt();
        QRectF rect(left, top, right, bottom);
        if (values[0] == "i") {
            insert(rect);
        } else if (values[0] == "r") {
            remove(rect);
        }
    }

    update();
    if (m_listId < m_list.size()) {
        int sleep = 1000;
        if (QCoreApplication::arguments().size() >= 3) {
            sleep = QCoreApplication::arguments().at(2).toInt();
        }
        QTimer::singleShot(sleep, this, &Canvas::replayStep);
    }
}

void Canvas::debug()
{
    m_rtree.debug();
}

void Canvas::paintTree(bool paintTree)
{
    m_paintTree = paintTree;
    update();
}

void Canvas::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(m_zoom, m_zoom);

    if (m_tool)
        m_tool->paint(p);

    QPen pen(Qt::black);
    p.setPen(pen);
    foreach (Data *data, m_rects) {
        data->paint(p);
    }

    if (m_paintTree) {
        m_rtree.paint(p);
    }

    foreach (Data *data, m_found) {
        QColor c(Qt::yellow);
        c.setAlphaF(0.1);
        QBrush brush(c);
        p.setBrush(brush);
        p.drawRect(data->boundingBox());
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *e)
{
    if (m_tool) {
        m_tool->mouseMoveEvent(e);
    }
}

void Canvas::mousePressEvent(QMouseEvent *e)
{
    if (m_tool) {
        m_tool->mousePressEvent(e);
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_tool) {
        m_tool->mouseReleaseEvent(e);
    }
}

void Canvas::selectInsertTool()
{
    m_tool = &m_createTool;
}

void Canvas::selectSelectTool()
{
    m_tool = &m_selectTool;
}

void Canvas::selectRemoveTool()
{
    m_tool = &m_removeTool;
}

MainWindow::MainWindow()
{
    m_canvas = new Canvas();
    setCentralWidget(m_canvas);
    m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    resize(640, 480);
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    setWindowTitle(tr("R-Tree Library Test Application"));

    m_canvas->repaint();

    repaint();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About test"), tr("R-Tree Library Test Application"));
}

void MainWindow::createActions()
{
    m_quitAct = new QAction(tr("&Quit"), this);
    m_quitAct->setShortcut(QKeySequence(tr("Ctrl+Q")));
    m_quitAct->setStatusTip(tr("Quit the application"));
    connect(m_quitAct, &QAction::triggered, this, &QWidget::close);

    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, &QAction::triggered, this, &MainWindow::about);

    m_aboutQtAct = new QAction(tr("About &Qt"), this);
    m_aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(m_aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    m_insertAct = new QAction(tr("&Insert"), this);
    m_insertAct->setStatusTip(tr("Insert Object"));
    connect(m_insertAct, &QAction::triggered, m_canvas, &Canvas::selectInsertTool);

    m_selectAct = new QAction(tr("&Select"), this);
    m_selectAct->setStatusTip(tr("Select Objects"));
    connect(m_selectAct, &QAction::triggered, m_canvas, &Canvas::selectSelectTool);

    m_removeAct = new QAction(tr("&Remove"), this);
    m_removeAct->setStatusTip(tr("Remove Object"));
    connect(m_removeAct, &QAction::triggered, m_canvas, &Canvas::selectRemoveTool);

    m_clearAct = new QAction(tr("&Clear"), this);
    m_clearAct->setStatusTip(tr("Clear Object"));
    connect(m_clearAct, &QAction::triggered, m_canvas, &Canvas::clear);

    m_replayAct = new QAction(tr("&Replay"), this);
    m_replayAct->setShortcut(QKeySequence(tr("Ctrl+R")));
    m_replayAct->setStatusTip(tr("Replay"));
    connect(m_replayAct, &QAction::triggered, m_canvas, &Canvas::replay);

    m_debugAct = new QAction(tr("&Debug"), this);
    m_debugAct->setShortcut(QKeySequence(tr("Ctrl+D")));
    m_debugAct->setStatusTip(tr("Debug"));
    connect(m_debugAct, &QAction::triggered, m_canvas, &Canvas::debug);

    m_paintTreeAct = new QAction(tr("&Paint Tree"), this);
    m_paintTreeAct->setShortcut(QKeySequence(tr("Ctrl+P")));
    m_paintTreeAct->setStatusTip(tr("Paint Tree"));
    m_paintTreeAct->setCheckable(true);
    connect(m_paintTreeAct, &QAction::toggled, m_canvas, &Canvas::paintTree);
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_replayAct);
    m_fileMenu->addAction(m_debugAct);
    m_fileMenu->addAction(m_paintTreeAct);
    m_fileMenu->addAction(m_quitAct);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_insertAct);
    m_editMenu->addAction(m_selectAct);
    m_editMenu->addAction(m_removeAct);
    m_editMenu->addAction(m_clearAct);
    menuBar()->addSeparator();

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAct);
    m_helpMenu->addAction(m_aboutQtAct);
}

void MainWindow::createToolBars()
{
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}
