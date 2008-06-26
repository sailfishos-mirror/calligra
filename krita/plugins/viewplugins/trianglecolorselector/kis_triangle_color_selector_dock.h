/*
 *  Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_TRIANGLE_COLOR_SELECTOR_DOCK_H_
#define _KIS_TRIANGLE_COLOR_SELECTOR_DOCK_H_

#include <QDockWidget>

class KoColor;
class KisView2;
class KoTriangleColorSelector;

class KisTriangleColorSelectorDock : public QDockWidget {
    Q_OBJECT
    public:
        KisTriangleColorSelectorDock( KisView2 *view );
    public slots:
        void colorChangedProxy(const QColor&);
        void setColorProxy( const KoColor& );
private:
    KoTriangleColorSelector* m_colorSelector;
    KisView2* m_view;
};


#endif
