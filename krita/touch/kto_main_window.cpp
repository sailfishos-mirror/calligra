/*
 * Copyright (C) 2012 Cyrille Berger <cberger@cberger.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 sure
 * USA
 */

#include "kto_main_window.h"

#include <QDebug>
#include <QDeclarativeView>
#include <QDeclarativeEngine>

#include <KGlobal>
#include <KComponentData>
#include <KStandardDirs>

#include "kto_canvas.h"
#include "kto_resource_information_list.h"
#include "kto_icon_provider.h"
#include <kis_doc2.h>
#include <kis_layer_manager.h>

KtoMainWindow::KtoMainWindow(QWidget* parent )
{
    m_view = new QDeclarativeView (this);
    setCentralWidget(m_view);
    
    KGlobal::mainComponent().dirs()->addResourceType("krita_touch_qml", "data", "kritatouch/qml/");

    m_view->engine()->addImageProvider(QLatin1String("kicon"), new KtoIconProvider);
    
    m_view->setSource(QUrl::fromLocalFile(KGlobal::mainComponent().dirs()->findResource("krita_touch_qml", "MainWindow.qml")));
    m_view->setResizeMode (QDeclarativeView::SizeRootObjectToView);
    
    m_canvas = m_view->rootObject()->findChild<KtoCanvas*>("canvas");
    Q_ASSERT(m_canvas);
    
    m_doc = new KisDoc2(0, this);
    m_canvas->setMainWindow(this);
}

KtoMainWindow::~KtoMainWindow()
{
}

KisDoc2* KtoMainWindow::document()
{
    return m_doc;
}

void KtoMainWindow::openFile (const QString& path)
{
    
}


#include "kto_main_window.moc"
