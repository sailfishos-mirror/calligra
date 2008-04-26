/* This file is part of the KDE project
 * Copyright (C) 2007-2008 Jan Hambrecht <jaham@gmx.net>
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

#ifndef _KARBONGRADIENTTOOL_H_
#define _KARBONGRADIENTTOOL_H_

#include <KoTool.h>
#include <QtGui/QGradient>
#include <QtCore/QMultiMap>

class GradientStrategy;
class VGradientTabWidget;
class QUndoCommand;
class KoShape;

/**
 * A tool for editing gradient backgrounds of shapes.
 * The gradients can be edited by moving gradient
 * handles directly on the canvas.
 */
class KarbonGradientTool : public KoTool
{
    Q_OBJECT
public:
    explicit KarbonGradientTool(KoCanvasBase *canvas);
    ~KarbonGradientTool();

    virtual void paint( QPainter &painter, const KoViewConverter &converter );
    virtual void repaintDecorations();

    virtual void mousePressEvent( KoPointerEvent *event ) ;
    virtual void mouseMoveEvent( KoPointerEvent *event );
    virtual void mouseReleaseEvent( KoPointerEvent *event );
    virtual void mouseDoubleClickEvent( KoPointerEvent *event );
    virtual void keyPressEvent(QKeyEvent *event);

    virtual void activate (bool temporary=false);
    virtual void deactivate();

protected:
    virtual QWidget * createOptionWidget();

private Q_SLOTS:
    virtual void resourceChanged( int key, const QVariant & res );
    void initialize();
    void gradientChanged();
private:
    QGradient * m_gradient;
    QMultiMap<KoShape*,GradientStrategy*> m_gradients; ///< the list of gradient strategies
    GradientStrategy * m_currentStrategy;  ///< the current editing strategy
    GradientStrategy * m_hoverStrategy; ///< the strategy the mouse hovers over
    VGradientTabWidget * m_gradientWidget;
    QUndoCommand * m_currentCmd;
    int m_oldSnapStrategies; ///< the previously enables snap strategies
};

#endif // _KARBONGRADIENTTOOL_H_
