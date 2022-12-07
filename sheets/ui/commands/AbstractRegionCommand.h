/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_ABSTRACT_REGION_COMMAND
#define CALLIGRA_SHEETS_ABSTRACT_REGION_COMMAND

#include <kundo2command.h>

#include "engine/Region.h"

#include "../sheets_ui_export.h"

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
class CALLIGRA_SHEETS_UI_EXPORT AbstractRegionCommand : public Region, public KUndo2Command
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
     * If \p registerUndo is \c true , this manipulator registers an
     * undo operation for the document.
     */
    void setRegisterUndo(bool registerUndo) {
        m_register = registerUndo;
    }

protected:
    /**
     * Processes \p element .
     * Invoked by performCommands() .
     */
    virtual bool process(Element *) {
        return true;
    }

    /**
     * Preprocessing of the region.
     */
    virtual bool preProcess() {
        return true;
    }

    /**
     * Undo the preprocessing of the region.
     */
    virtual bool preProcessUndo() {
        return true;
    }

    /**
     * Processes the region. Calls process(rect, sheet).
     */
    virtual bool performCommands();

    /**
     * Perform actions that do not generate CellStorage commands.
     */
    virtual bool performNonCommandActions() {
        return true;
    }

    /**
     * Undo actions that were not generated by CellStorage commands.
     */
    virtual bool undoNonCommandActions() {
        return true;
    }

    /**
     * Postprocessing of the region.
     */
    virtual bool postProcess() {
        return true;
    }

    /**
     * Undo the postprocessing of the region.
     */
    virtual bool postProcessUndo() {
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
    bool    m_firstrun  : 1;
    bool    m_register  : 1;
    bool    m_success   : 1;
    bool    m_checkLock : 1;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_ABSTRACT_REGION_COMMAND