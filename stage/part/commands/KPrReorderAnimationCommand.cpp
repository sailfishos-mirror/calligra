/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Paul Mendez <paulestebanms@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (  at your option ) any later version.
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

#include "KPrReorderAnimationCommand.h"

#include "KPrShapeAnimations.h"
#include "animations/KPrShapeAnimation.h"

KPrReorderAnimationCommand::KPrReorderAnimationCommand(KPrShapeAnimations *shapeAnimationsModel,
                                                       KPrShapeAnimation *oldAnimation,
                                                       KPrShapeAnimation *newAnimation,
                                                       KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_shapeAnimationsModel(shapeAnimationsModel)
    , m_oldAnimation(oldAnimation)
    , m_newAnimation(newAnimation)
{
    setText(kundo2_i18n("Reorder animations"));
}

KPrReorderAnimationCommand::~KPrReorderAnimationCommand() = default;

void KPrReorderAnimationCommand::redo()
{
    m_shapeAnimationsModel->swapAnimations(m_oldAnimation, m_newAnimation);
}

void KPrReorderAnimationCommand::undo()
{
    m_shapeAnimationsModel->swapAnimations(m_newAnimation, m_oldAnimation);
}
