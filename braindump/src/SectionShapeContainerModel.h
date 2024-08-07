/*
 *  SPDX-FileCopyrightText: 2006-2007 Thomas Zander <zander@kde.org>
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
#ifndef SECTIONSHAPECONTAINERMODEL_H
#define SECTIONSHAPECONTAINERMODEL_H

#include "KoShapeContainerModel.h"

#include <Section.h>

class SectionShapeContainerModel : public KoShapeContainerModel
{
public:
    explicit SectionShapeContainerModel(Section *_section);
    ~SectionShapeContainerModel();
    void add(KoShape *child) override;
    void setClipped(const KoShape *, bool) override;
    bool isClipped(const KoShape *) const override;
    void setInheritsTransform(const KoShape *, bool) override;
    bool inheritsTransform(const KoShape *) const override;
    void remove(KoShape *child) override;
    int count() const override;
    QList<KoShape *> shapes() const override;
    void containerChanged(KoShapeContainer *, KoShape::ChangeType type) override;
    void childChanged(KoShape *, KoShape::ChangeType) override;
    bool isChildLocked(const KoShape *child) const override;

public:
    /**
     * Determine whether to update or not the layout.
     */
    void setUpdateLayout(bool v);

private: // members
    QList<KoShape *> m_members;
    Section *m_section;
    bool m_updateLayout;
};

#endif
