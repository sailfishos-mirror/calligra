/* This file is part of the KDE libraries
   Copyright (c) 1998 Stefan Taferner
                    2001/2003 thierry lorthiois (lorthioist@wanadoo.fr)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef _KOWMFSTACK_H_
#define _KOWMFSTACK_H_

#include <qpen.h>
#include <qcolor.h>
#include <qfont.h>
#include <qbrush.h>
#include <qpixmap.h>

class KoWmfRead;

/**
 * WMF file allows manipulation on a stack of object.
 * It's possile to create, delete or select an object.
 */
class KoWmfHandle
{
public:
    virtual void apply( KoWmfRead * ) = 0;
};

class KoWmfBrushHandle: public KoWmfHandle
{
public:
    virtual ~KoWmfBrushHandle() {};
    virtual void apply( KoWmfRead * );
    QBrush brush;
};

class KoWmfPenHandle: public KoWmfHandle
{
public:
    virtual ~KoWmfPenHandle() {};
    virtual void apply( KoWmfRead * );
    QPen pen;
};

class KoWmfPatternBrushHandle: public KoWmfHandle
{
public:
    virtual ~KoWmfPatternBrushHandle() {};
    virtual void apply( KoWmfRead * );
    QBrush brush;
    QPixmap image;
};

class KoWmfFontHandle: public KoWmfHandle
{
public:
    virtual ~KoWmfFontHandle() {};
    virtual void apply( KoWmfRead * );
    QFont font;
    int rotation;
};

#endif
