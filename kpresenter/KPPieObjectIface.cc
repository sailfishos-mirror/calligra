/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "KPPieObjectIface.h"
#include "kppieobject.h"
#include "kpobject.h"

#include <kapplication.h>
#include <dcopclient.h>
#include <kdebug.h>

KPPieObjectIface::KPPieObjectIface( KPPieObject *_obj )
    : KPresenterObjectIface(_obj)

{
   obj = _obj;
}

void KPPieObjectIface::setPieAngle( int _p_angle )
{
    obj->setPieAngle(_p_angle);
}

void KPPieObjectIface::setPieLength( int _p_len )
{
    obj->setPieLength(_p_len);
}

int KPPieObjectIface::pieAngle() const
{
    return obj->getPieAngle();
}

int KPPieObjectIface::pieLength() const
{
    return obj->getPieLength();
}

void KPPieObjectIface::setPieType( const QString & type )
{
    if( type =="Pie")
    {
        obj->setPieType(PT_PIE);
    }
    else if(type=="Arc")
    {
        obj->setPieType(PT_ARC);
    }
    else if(type=="Chord")
    {
        obj->setPieType(PT_CHORD);
    }
}

void KPPieObjectIface::setLineBegin( const QString & type)
{
    if(type=="NORMAL")
        obj->setLineBegin(L_NORMAL );
    else if(type=="ARROW")
        obj->setLineBegin(L_ARROW );
    else if(type=="SQUARE")
        obj->setLineBegin(L_SQUARE );
    else if(type=="CIRCLE")
        obj->setLineBegin(L_CIRCLE );
    else
        kdDebug()<<"Error in KPPieObjectIface::setLineBegin\n";

}

void KPPieObjectIface::setLineEnd( const QString & type)
{
    if(type=="NORMAL")
        obj->setLineEnd(L_NORMAL );
    else if(type=="ARROW")
        obj->setLineEnd(L_ARROW );
    else if(type=="SQUARE")
        obj->setLineEnd(L_SQUARE );
    else if(type=="CIRCLE")
        obj->setLineEnd(L_CIRCLE );
    else
        kdDebug()<<"Error in KPPieObjectIface::setLineEnd\n";

}
