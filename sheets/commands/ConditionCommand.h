/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_CONDITION_COMMAND
#define CALLIGRA_SHEETS_CONDITION_COMMAND

#include <QLinkedList>

#include "AbstractRegionCommand.h"
#include "Condition.h"

namespace Calligra
{
namespace Sheets
{

/**
 * \class ConditionCommand
 * \ingroup Commands
 * \brief Adds/Removes conditional formatting to/of a cell region.
 */
class ConditionCommand : public AbstractRegionCommand
{
public:
    ConditionCommand();
    void setConditionList(const QLinkedList<Conditional>& list);

protected:
    bool process(Element* element) override;
    bool mainProcessing() override;

private:
    Conditions m_conditions;
    QList< QPair<QRectF, Conditions> > m_undoData;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_CONDITION_COMMAND
