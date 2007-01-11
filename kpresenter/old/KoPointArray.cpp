// -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
/* This file is part of the KDE project
   Copyright (C) 2001 Laurent MONTEL <lmontel@mandrakesoft.com>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KoPointArray.h"
#include <stdarg.h>
#include <KoZoomHandler.h>
//Added by qt3to4:
#include <Q3MemArray>
#include <Q3PointArray>

void KoPointArray::translate( double dx, double dy )
{
    register QPointF *p = data();
    register int i = size();
    QPointF pt( dx, dy );
    while ( i-- ) {
        *p += pt;
        p++;
    }
}

void KoPointArray::point( uint index, double *x, double *y ) const
{
    QPointF p = Q3MemArray<QPointF>::at( index );
    if ( x )
        *x = (double)p.x();
    if ( y )
        *y = (double)p.y();
}

QPointF KoPointArray::point( uint index ) const
{ // #### index out of bounds
    return Q3MemArray<QPointF>::at( index );
}

void KoPointArray::setPoint( uint index, double x, double y )
{ // #### index out of bounds
    Q3MemArray<QPointF>::at( index ) = QPointF( x, y );
}



bool KoPointArray::putPoints( int index, int nPoints, double firstx, double firsty,
                              ... )
{
    va_list ap;
    if ( index + nPoints > (int)size() ) {  // extend array
        if ( !resize(index + nPoints) )
            return false;
    }
    if ( nPoints <= 0 )
        return true;
    setPoint( index, firstx, firsty );      // set first point
    int i = index + 1;
    double x, y;
    nPoints--;
    va_start( ap, firsty );
    while ( nPoints-- ) {
        x = va_arg( ap, double );
        y = va_arg( ap, double );
        setPoint( i++, x, y );
    }
    va_end( ap );
    return true;
}

void split(const double *p, double *l, double *r)
{
    double tmpx;
    double tmpy;

    l[0] =  p[0];
    l[1] =  p[1];
    r[6] =  p[6];
    r[7] =  p[7];

    l[2] = (p[0]+ p[2])/2;
    l[3] = (p[1]+ p[3])/2;
    tmpx = (p[2]+ p[4])/2;
    tmpy = (p[3]+ p[5])/2;
    r[4] = (p[4]+ p[6])/2;
    r[5] = (p[5]+ p[7])/2;

    l[4] = (l[2]+ tmpx)/2;
    l[5] = (l[3]+ tmpy)/2;
    r[2] = (tmpx + r[4])/2;
    r[3] = (tmpy + r[5])/2;

    l[6] = (l[4]+ r[2])/2;
    l[7] = (l[5]+ r[3])/2;
    r[0] = l[6];
    r[1] = l[7];
}

// Based on:
//
//   A Fast 2D Point-On-Line Test
//   by Alan Paeth
//   from "Graphics Gems", Academic Press, 1990
static
int pnt_on_line( const int* p, const int* q, const int* t )
{
/*
 * given a line through P:(px,py) Q:(qx,qy) and T:(tx,ty)
 * return 0 if T is not on the line through      <--P--Q-->
 *        1 if T is on the open ray ending at P: <--P
 *        2 if T is on the closed interior along:   P--Q
 *        3 if T is on the open ray beginning at Q:    Q-->
 *
 * Example: consider the line P = (3,2), Q = (17,7). A plot
 * of the test points T(x,y) (with 0 mapped onto '.') yields:
 *
 *     8| . . . . . . . . . . . . . . . . . 3 3
 *  Y  7| . . . . . . . . . . . . . . 2 2 Q 3 3    Q = 2
 *     6| . . . . . . . . . . . 2 2 2 2 2 . . .
 *  a  5| . . . . . . . . 2 2 2 2 2 2 . . . . .
 *  x  4| . . . . . 2 2 2 2 2 2 . . . . . . . .
 *  i  3| . . . 2 2 2 2 2 . . . . . . . . . . .
 *  s  2| 1 1 P 2 2 . . . . . . . . . . . . . .    P = 2
 *     1| 1 1 . . . . . . . . . . . . . . . . .
 *      +--------------------------------------
 *        1 2 3 4 5 X-axis 10        15      19
 *
 * Point-Line distance is normalized with the Infinity Norm
 * avoiding square-root code and tightening the test vs the
 * Manhattan Norm. All math is done on the field of integers.
 * The latter replaces the initial ">= MAX(...)" test with
 * "> (ABS(qx-px) + ABS(qy-py))" loosening both inequality
 * and norm, yielding a broader target line for selection.
 * The tightest test is employed here for best discrimination
 * in merging collinear (to grid coordinates) vertex chains
 * into a larger, spanning vectors within the Lemming editor.
 */

    // if all points are coincident, return condition 2 (on line)
    if(q[0]==p[0] && q[1]==p[1] && q[0]==t[0] && q[1]==t[1]) {
        return 2;
    }

    if ( QABS((q[1]-p[1])*(t[0]-p[0])-(t[1]-p[1])*(q[0]-p[0])) >=
         (qMax(QABS(q[0]-p[0]), QABS(q[1]-p[1])))) return 0;

    if (((q[0]<p[0])&&(p[0]<t[0])) || ((q[1]<p[1])&&(p[1]<t[1])))
        return 1 ;
    if (((t[0]<p[0])&&(p[0]<q[0])) || ((t[1]<p[1])&&(p[1]<q[1])))
        return 1 ;
    if (((p[0]<q[0])&&(q[0]<t[0])) || ((p[1]<q[1])&&(q[1]<t[1])))
        return 3 ;
    if (((t[0]<q[0])&&(q[0]<p[0])) || ((t[1]<q[1])&&(q[1]<p[1])))
        return 3 ;

    return 2 ;
}

static
void polygonizeQBezier( double* acc, int& accsize, const double ctrl[],
                        int maxsize )
{
    if ( accsize > maxsize / 2 )
    {
        // This never happens in practice.

        if ( accsize >= maxsize-4 )
            return;
        // Running out of space - approximate by a line.
        acc[accsize++] = ctrl[0];
        acc[accsize++] = ctrl[1];
        acc[accsize++] = ctrl[6];
        acc[accsize++] = ctrl[7];
        return;
    }

    //intersects:
    double l[8];
    double r[8];
    split( ctrl, l, r);

    // convert to integers for line condition check
    int c0[2]; c0[0] = int(ctrl[0]); c0[1] = int(ctrl[1]);
    int c1[2]; c1[0] = int(ctrl[2]); c1[1] = int(ctrl[3]);
    int c2[2]; c2[0] = int(ctrl[4]); c2[1] = int(ctrl[5]);
    int c3[2]; c3[0] = int(ctrl[6]); c3[1] = int(ctrl[7]);

    // #### Duplication needed?
    if ( QABS(c1[0]-c0[0]) <= 1 && QABS(c1[1]-c0[1]) <= 1
         && QABS(c2[0]-c0[0]) <= 1 && QABS(c2[1]-c0[1]) <= 1
         && QABS(c3[0]-c1[0]) <= 1 && QABS(c3[1]-c0[1]) <= 1 )
    {
        // Approximate by one line.
        // Dont need to write last pt as it is the same as first pt
        // on the next segment
        acc[accsize++] = l[0];
        acc[accsize++] = l[1];
        return;
    }

    if ( ( pnt_on_line( c0, c3, c1 ) == 2 && pnt_on_line( c0, c3, c2 ) == 2 )
         || ( QABS(c1[0]-c0[0]) <= 1 && QABS(c1[1]-c0[1]) <= 1
              && QABS(c2[0]-c0[0]) <= 1 && QABS(c2[1]-c0[1]) <= 1
              && QABS(c3[0]-c1[0]) <= 1 && QABS(c3[1]-c0[1]) <= 1 ) )
    {
        // Approximate by one line.
        // Dont need to write last pt as it is the same as first pt
        // on the next segment
        acc[accsize++] = l[0];
        acc[accsize++] = l[1];
        return;
    }

    // Too big and too curved - recusively subdivide.
    polygonizeQBezier( acc, accsize, l, maxsize );
    polygonizeQBezier( acc, accsize, r, maxsize );
}


QRectF KoPointArray::boundingRect() const
{
    if ( isEmpty() )
        return QRectF( 0, 0, 0, 0 );        // null rectangle
    register QPointF *pd = data();
    double minx, maxx, miny, maxy;
    minx = maxx = pd->x();
    miny = maxy = pd->y();
    pd++;
    for ( int i=1; i<(int)size(); i++ ) {   // find min+max x and y
        if ( pd->x() < minx )
            minx = pd->x();
        else if ( pd->x() > maxx )
            maxx = pd->x();
        if ( pd->y() < miny )
            miny = pd->y();
        else if ( pd->y() > maxy )
            maxy = pd->y();
        pd++;
    }
#ifdef __GNUC__
#warning "kde4 verify it!"    
#endif
    return QRectF( QPointF(minx,miny), QSizeF(maxx-minx,maxy-miny) );
}


KoPointArray KoPointArray::cubicBezier() const
{
    if ( size() != 4 ) {
#if defined(QT_CHECK_RANGE)
        qWarning( "QPointArray::bezier: The array must have 4 control points" );
#endif
        KoPointArray pa;
        return pa;
    } else {
        QRectF r = boundingRect();
        int m = (int)(4+2*qMax(r.width(),r.height()));
        double *p = new double[m];
        double ctrl[8];
        int i;
        for (i=0; i<4; i++) {
            ctrl[i*2] = at(i).x();
            ctrl[i*2+1] = at(i).y();
        }
        int len=0;
        polygonizeQBezier( p, len, ctrl, m );
        KoPointArray pa((len/2)+1); // one extra point for last point on line
        int j=0;
        for (i=0; j<len; i++) {
            double x = qRound(p[j++]);
            double y = qRound(p[j++]);
            pa[i] = QPointF(x,y);
        }
        // add last pt on the line, which will be at the last control pt
        pa[(int)pa.size()-1] = at(3);
        delete[] p;

        return pa;
    }
}

Q3PointArray KoPointArray::zoomPointArray( const KoZoomHandler* zoomHandler ) const
{
    Q3PointArray tmpPoints(size());
    for ( uint i= 0; i<size();i++ ) {
        QPointF p = at( i );
        tmpPoints.putPoints( i, 1, zoomHandler->zoomItXOld(p.x()),zoomHandler->zoomItYOld(p.y()) );
    }
    return tmpPoints;
}

Q3PointArray KoPointArray::zoomPointArray( const KoZoomHandler* zoomHandler, int penWidth ) const
{
    double fx;
    double fy;
    QSizeF ext = boundingRect().size();
    int pw = zoomHandler->zoomItXOld( penWidth ) / 2;

    fx = (double)( zoomHandler->zoomItXOld(ext.width()) - 2 * pw ) / ext.width();
    fy = (double)( zoomHandler->zoomItYOld(ext.height()) - 2 * pw ) / ext.height();

    unsigned int index = 0;
    Q3PointArray tmpPoints;
    KoPointArray::ConstIterator it;
    for ( it = begin(); it != end(); ++it, ++index ) {
        int tmpX = qRound((*it).x() * fx + pw);
        int tmpY = qRound((*it).y() * fy + pw);

        tmpPoints.putPoints( index, 1, tmpX, tmpY );
    }
    return tmpPoints;
}
