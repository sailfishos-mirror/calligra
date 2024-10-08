/* This file is part of the wvWare 2 project
   SPDX-FileCopyrightText: 2002-2003 Werner Trobin <trobin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02111-1307, USA.
*/

#include "paragraphproperties.h"
#include "lists.h"

using namespace wvWare;

ParagraphProperties::ParagraphProperties()
    : m_listInfo(nullptr)
{
}

ParagraphProperties::ParagraphProperties(const Word97::PAP &pap)
    : m_pap(pap)
    , m_listInfo(nullptr)
{
}

ParagraphProperties::ParagraphProperties(const ParagraphProperties &rhs)
    : Shared(rhs)
    , m_pap(rhs.pap())
    , m_listInfo(nullptr)
{
    if (rhs.listInfo())
        m_listInfo = new ListInfo(*rhs.listInfo());
}

ParagraphProperties::~ParagraphProperties()
{
    delete m_listInfo;
}

Word97::PAP &ParagraphProperties::pap()
{
    return m_pap;
}

const Word97::PAP &ParagraphProperties::pap() const
{
    return m_pap;
}

const ListInfo *ParagraphProperties::listInfo() const
{
    return m_listInfo;
}

void ParagraphProperties::setBulletPictureName(const QString &name)
{
    m_listInfo->setBulletPictureName(name);
}

void ParagraphProperties::createListInfo(ListInfoProvider &listInfoProvider, Word97::CHP &chp)
{
    if (m_listInfo || !listInfoProvider.isValid(m_pap.ilfo, m_pap.nLvlAnm)) {
        return;
    }
    m_listInfo = new ListInfo(m_pap, chp, listInfoProvider);
}
