/*
 *  SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _STATETOOL_H_
#define _STATETOOL_H_

#include <KoToolBase.h>

class StateShape;
class KoShape;

class StateTool : public KoToolBase
{
    Q_OBJECT
public:
    explicit StateTool(KoCanvasBase *canvas);
    ~StateTool();

    void activate(ToolActivation toolActivation, const QSet<KoShape *> &shapes) override;
    void paint(QPainter &painter, const KoViewConverter &converter) override;
    void mousePressEvent(KoPointerEvent *event) override;
    void mouseMoveEvent(KoPointerEvent *event) override;
    void mouseReleaseEvent(KoPointerEvent *event) override;

Q_SIGNALS:
    void shapeChanged(StateShape *);

protected:
    QList<QPointer<QWidget>> createOptionWidgets() override;

private:
    StateShape *m_currentShape;
};

#endif
