/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_ABSTRACT_REGION_COMMAND
#define CALLIGRA_SHEETS_ABSTRACT_REGION_COMMAND

#include <kundo2command.h>

#include "Region.h"

#include "sheets_common_export.h"

class KoCanvasBase;

namespace Calligra
{
namespace Sheets
{
class Sheet;

/**
 * \class AbstractRegionCommand
 * \ingroup Commands
 * \brief Abstract base class for all region related operations.
 */
class CALLIGRA_SHEETS_COMMON_EXPORT AbstractRegionCommand : public Region, public KUndo2Command
{
public:
    /**
     * Constructor.
     */
    explicit AbstractRegionCommand(KUndo2Command *parent = 0);

    /**
     * Destructor.
     */
    ~AbstractRegionCommand() override;

    /**
     * \return the Sheet this AbstractRegionCommand works on
     */
    Sheet* sheet() const {
        return m_sheet;
    }

    /**
     * Sets \p sheet to be the Sheet to work on.
     */
    void setSheet(Sheet* sheet) {
        m_sheet = sheet;
    }

    /**
     * Executes the actual operation and adds the manipulator to the undo history, if desired.
     * \return \c true if the command was executed successfully
     * \return \c false if the command fails, was already executed once or is not approved
     * \see setRegisterUndo, isApproved
     */
    virtual bool execute(KoCanvasBase* canvas = 0);

    /**
     * Executes the actual operation.
     */
    void redo() override;

    /**
     * Executes the actual operation in reverse order.
     */
    void undo() override;

    /**
     * Sets reverse mode to \b reverse .
     * \see redo
     * \see undo
     */
    virtual void setReverse(bool reverse) {
        m_reverse = reverse;
    }

    /**
     * If \p registerUndo is \c true , this manipulator registers an
     * undo operation for the document.
     */
    void setRegisterUndo(bool registerUndo) {
        m_register = registerUndo;
    }

protected:
    /**
     * Processes \p element , a Region::Point or a Region::Range .
     * Invoked by mainProcessing() .
     */
    virtual bool process(Element*) {
        return true;
    }

    /**
     * Preprocessing of the region.
     */
    virtual bool preProcessing() {
        return true;
    }

    /**
     * Processes the region. Calls process(Element*).
     */
    virtual bool mainProcessing();

    /**
     * Postprocessing of the region.
     */
    virtual bool postProcessing() {
        return true;
    }

    /**
     * Checks all cells, that should be processed, for protection and matrix locks.
     * \return \c true if execution is approved
     * \return \c false otherwise
     */
    bool isApproved() const;

protected:
    Sheet*  m_sheet;
    bool    m_reverse   : 1;
    bool    m_firstrun  : 1;
    bool    m_register  : 1;
    bool    m_success   : 1;
    bool    m_checkLock : 1;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_ABSTRACT_REGION_COMMAND
