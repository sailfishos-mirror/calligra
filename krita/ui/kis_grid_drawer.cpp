/*
 * This file is part of Krita
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#include "kis_grid_drawer.h"

#include <config.h>
#include <config-krita.h>

#ifdef HAVE_OPENGL
#include <qgl.h>
#endif

#include "kis_config.h"
#include "kis_image.h"
#include "kis_perspective_grid.h"
#include "kis_perspective_grid_manager.h"


Qt::PenStyle GridDrawer::gs2style(quint32 s)
{
    switch(s)
    {
        case 1:
            return Qt::DashLine;
        case 2:
            return Qt::DotLine;
        case 3:
            return Qt::DashDotLine;
        case 4:
            return Qt::DashDotDotLine;
        default:
            return Qt::SolidLine;
    }
}

void GridDrawer::drawPerspectiveGrid(KisImageSP image, const QRect& /*wr*/, const KisSubPerspectiveGrid* grid)
{
    KisConfig cfg;
    QPen mainPen = QPen ( cfg.getGridMainColor(), 1, gs2style( cfg.getGridMainStyle() ) );
    QPen subdivisionPen =  QPen ( cfg.getGridSubdivisionColor(), 1, gs2style( cfg.getGridSubdivisionStyle() ) );
    setPen(subdivisionPen );
    // 1 -> top-left corner
    // 2 -> top-right corner
    // 3 -> bottom-right corner
    // 4 -> bottom-left corner
    // d12 line from top-left to top-right
    // note that the notion of top-left is purely theorical
    KisPerspectiveMath::LineEquation d12 = KisPerspectiveMath::computeLineEquation( grid->topLeft().data(), grid->topRight().data() ) ;
    QPointF v12 = QPointF(*grid->topLeft() - *grid->topRight());
    v12.setX( v12.x() / grid->subdivisions()); v12.setY( v12.y() / grid->subdivisions() );
    KisPerspectiveMath::LineEquation d23 = KisPerspectiveMath::computeLineEquation( grid->topRight().data(), grid->bottomRight().data() );
    QPointF v23 = QPointF(*grid->topRight() - *grid->bottomRight());
    v23.setX( v23.x() / grid->subdivisions()); v23.setY( v23.y() / grid->subdivisions() );
    KisPerspectiveMath::LineEquation d34 = KisPerspectiveMath::computeLineEquation( grid->bottomRight().data(), grid->bottomLeft().data() );
    KisPerspectiveMath::LineEquation d41 = KisPerspectiveMath::computeLineEquation( grid->bottomLeft().data(), grid->topLeft().data() );
    
    QPointF horizVanishingPoint = KisPerspectiveMath::computeIntersection(d12,d34);
    QPointF vertVanishingPoint = KisPerspectiveMath::computeIntersection(d23,d41);
    
    for(uint i = 1; i < grid->subdivisions(); i ++)
    {
        QPointF pol1 = *grid->topRight() + i * v12;
        KisPerspectiveMath::LineEquation d1 = KisPerspectiveMath::computeLineEquation( &pol1, &vertVanishingPoint );
        QPointF pol1b =  KisPerspectiveMath::computeIntersection(d1,d34);
        drawLine( pol1.toPoint(), pol1b.toPoint() );
        
        QPointF pol2 = *grid->bottomRight() + i * v23;
        KisPerspectiveMath::LineEquation d2 = KisPerspectiveMath::computeLineEquation( &pol2, &horizVanishingPoint );
        QPointF pol2b = KisPerspectiveMath::computeIntersection(d2,d41);
        drawLine( pol2.toPoint(), pol2b.toPoint() );
    }
    setPen(mainPen);
    drawLine( grid->topLeft().data(), grid->topRight().data() );
    drawLine( grid->topRight().data(), grid->bottomRight().data() );
    drawLine( grid->bottomRight().data(), grid->bottomLeft().data() );
    drawLine( grid->bottomLeft().data(), grid->topLeft().data() );
}

void GridDrawer::drawGrid(KisImageSP image, const QRect& wr)
{
    KisConfig cfg;

    quint32 offsetx = cfg.getGridOffsetX();
    quint32 offsety = cfg.getGridOffsetY();
    quint32 hspacing = cfg.getGridHSpacing();
    quint32 vspacing = cfg.getGridVSpacing();
    quint32 subdivision = cfg.getGridSubdivisions() - 1;
    //double ihspsub = hspacing / (double)subdivision;
    //double ivspsub = hspacing / (double)subdivision;

    qint32 imageWidth = image->width();
    qint32 imageHeight = image->height();

    // Draw vertical line
    QPen mainPen = QPen ( cfg.getGridMainColor(), 1, gs2style( cfg.getGridMainStyle() ) );
    QPen subdivisionPen = QPen ( cfg.getGridSubdivisionColor(), 1, gs2style( cfg.getGridSubdivisionStyle() ) );
    quint32 i = 0;
    for( qint32 x = offsetx; x <= wr.right(); x +=hspacing)
    {
        if( i == subdivision )
        {
            setPen(mainPen);
            i = 0;
        } else {
            setPen(subdivisionPen);
            i++;
        }
        if( x >= wr.x() )
        {
            // Always draw the full line otherwise the line stippling varies
            // with the location of wr and we get glitchy patterns.
            drawLine(x, 0, x, imageHeight);
        }
    }
    // Draw horizontal line
    i = 0;
    for( qint32 y = offsety; y <= wr.bottom(); y +=vspacing)
    {
        if( i == subdivision )
        {
            setPen(mainPen);
            i = 0;
        } else {
            setPen(subdivisionPen);
            i++;
        }
        if( y >= wr.y() )
        {
            drawLine(0, y, imageWidth, y);
        }
    }
}

OpenGLGridDrawer::OpenGLGridDrawer()
{
#ifdef HAVE_OPENGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);
#endif
}

OpenGLGridDrawer::~OpenGLGridDrawer()
{
#ifdef HAVE_OPENGL
    glPopAttrib();
#endif
}

void OpenGLGridDrawer::setPen(const QPen& pen)
{
#ifdef HAVE_OPENGL
    Qt::PenStyle penStyle = pen.style();

    if (penStyle == Qt::SolidLine) {
        glDisable(GL_LINE_STIPPLE);
    } else {
        GLushort lineStipple;

        switch (penStyle) {
        case Qt::NoPen:
            lineStipple = 0;
            break;
        default:
        case Qt::SolidLine:
            lineStipple = 0xffff;
            break;
        case Qt::DashLine:
            lineStipple = 0x3fff;
            break;
        case Qt::DotLine:
            lineStipple = 0x3333;
            break;
        case Qt::DashDotLine:
            lineStipple = 0x33ff;
            break;
        case Qt::DashDotDotLine:
            lineStipple = 0x333f;
            break;
        }

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, lineStipple);
    }

    QColor penColor = pen.color();

    glColor3ub(penColor.red(), penColor.green(), penColor.blue());
#else
    Q_UNUSED(pen);
#endif
}

void OpenGLGridDrawer::drawLine(qint32 x1, qint32 y1, qint32 x2, qint32 y2)
{
#ifdef HAVE_OPENGL
    glBegin(GL_LINES);
    glVertex2i(x1, y1);
    glVertex2i(x2, y2);
    glEnd();
#else
    Q_UNUSED(x1);
    Q_UNUSED(y1);
    Q_UNUSED(x2);
    Q_UNUSED(y2);
#endif
}
