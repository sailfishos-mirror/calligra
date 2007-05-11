
/****************************************************************************
 ** Copyright (C) 2001-2006 Klarälvdalens Datakonsult AB.  All rights reserved.
 **
 ** This file is part of the KD Chart library.
 **
 ** This file may be distributed and/or modified under the terms of the
 ** GNU General Public License version 2 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.
 **
 ** Licensees holding valid commercial KD Chart licenses may use this file in
 ** accordance with the KD Chart Commercial License Agreement provided with
 ** the Software.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** See http://www.kdab.net/kdchart for
 **   information about KD Chart Commercial License Agreements.
 **
 ** Contact info@kdab.net if any conditions of this
 ** licensing are not clear to you.
 **
 **********************************************************************/
#include "KDChartWidgetSerializer.h"
#include <QDebug>

#include <QtDesignerUI4>

using namespace KDChart;

WidgetSerializer::WidgetSerializer( )
{
}

WidgetSerializer::~WidgetSerializer( )
{
}

bool KDChart::WidgetSerializer::saveExtraInfo( const Widget * widget, DomWidget * ui_widget )
{
    Q_UNUSED( widget )
    QList<DomItem*> items;
    DomItem* item = new DomItem;
    item->setText(QLatin1String("Widget Details"));
    items.append( item );
    ui_widget->setElementItem( items );
    return true;
}


bool KDChart::WidgetSerializer::loadExtraInfo( Widget * widget, DomWidget * ui_widget )
{
    Q_UNUSED( widget )
    Q_UNUSED( ui_widget )
    return true;
}

