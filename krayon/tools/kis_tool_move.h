/*
 *  movetool.h - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __movetool_h__
#define __movetool_h__

#include <qpoint.h>

#include "kis_tool.h"
#include "kis_undo.h"

class MoveCommand : public KisCommand
{
public:

  MoveCommand( KisDoc *_doc, int _layer, QPoint _oldpos, QPoint _newpos );

  virtual void execute();
  virtual void unexecute();

private:

  void moveTo( QPoint _pos );

  int m_layer;
  QPoint m_oldPos;
  QPoint m_newPos;
};

class MoveTool : public KisTool
{
public:

  MoveTool( KisDoc *doc, KisView *view);
  ~MoveTool();

  virtual QString toolName() { return QString( "MoveTool" ); }

  virtual void mousePress( QMouseEvent *e ); 
  virtual void mouseMove( QMouseEvent *e );
  virtual void mouseRelease(QMouseEvent *e );

  void setCursor();

protected:

  QPoint m_dragStart;
  QPoint m_dragPosition;
  QPoint m_layerStart;
  QPoint m_layerPosition;
  bool   m_dragging;
};

#endif //__movetool_h__
