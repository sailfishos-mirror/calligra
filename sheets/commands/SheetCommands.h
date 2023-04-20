/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004 Ariya Hidayat <ariya@kde.org>
   SPDX-FileCopyrightText: 2004 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_SHEET_COMMANDS
#define CALLIGRA_SHEETS_SHEET_COMMANDS

#include <QString>
#include <kundo2command.h>

namespace Calligra
{
namespace Sheets
{
class Map;
class Sheet;

/**
 * \ingroup Commands
 * \brief Renames a sheet.
 */
class RenameSheetCommand : public KUndo2Command
{
public:
    RenameSheetCommand(Sheet* sheet, const QString &name);

    void redo() override;
    void undo() override;

protected:
    Sheet* sheet;
    QString oldName;
    QString newName;
};


/**
 * \ingroup Commands
 * \brief Hides a sheet.
 */
class HideSheetCommand : public KUndo2Command
{
public:
    explicit HideSheetCommand(Sheet* sheet);

    void redo() override;
    void undo() override;

protected:
    Map* map;
    QString sheetName;
};


/**
 * \ingroup Commands
 * \brief Shows a hidden sheet.
 */
class ShowSheetCommand : public KUndo2Command
{
public:
    explicit ShowSheetCommand(Sheet* sheet, KUndo2Command* parent = 0);

    void redo() override;
    void undo() override;

protected:
    Map* map;
    QString sheetName;
};


/**
 * \ingroup Commands
 * \brief Adds a sheet.
 */
class AddSheetCommand : public KUndo2Command
{
public:
    explicit AddSheetCommand(Sheet* sheet);

    void redo() override;
    void undo() override;

protected:
    Sheet*  m_sheet;
    bool    m_firstrun;
};


/**
 * \ingroup Commands
 * \brief Duplicates a sheet.
 */
class DuplicateSheetCommand : public KUndo2Command
{
public:
    explicit DuplicateSheetCommand();

    void setSheet(Sheet* sheet);

    void redo() override;
    void undo() override;

protected:
    Sheet* m_oldSheet;
    Sheet* m_newSheet;
    bool m_firstrun;
};


/**
 * \ingroup Commands
 * \brief Removes a sheet.
 */
class RemoveSheetCommand : public KUndo2Command
{
public:
    explicit RemoveSheetCommand(Sheet* sheet);

    void redo() override;
    void undo() override;

protected:
    Sheet* sheet;
    Map* map;
};


/**
 * \ingroup Commands
 * \brief Changes sheet properties.
 */
class SheetPropertiesCommand : public KUndo2Command
{
public:
    explicit SheetPropertiesCommand(Sheet *sheet);
    void setLayoutDirection(Qt::LayoutDirection direction);
    void setAutoCalculationEnabled(bool b);
    void setShowGrid(bool b);
    void setShowPageOutline(bool b);
    void setShowFormula(bool b);
    void setHideZero(bool b);
    void setShowFormulaIndicator(bool b);
    void setShowCommentIndicator(bool b);
    void setColumnAsNumber(bool b);
    void setLcMode(bool b);
    void setCapitalizeFirstLetter(bool b);

    void redo() override;
    void undo() override;

protected:
    Sheet* sheet;
    Map* map;
    Qt::LayoutDirection oldDirection, newDirection;
    bool oldAutoCalc, newAutoCalc;
    bool oldShowGrid, newShowGrid;
    bool oldShowPageOutline, newShowPageOutline;
    bool oldShowFormula, newShowFormula;
    bool oldHideZero, newHideZero;
    bool oldShowFormulaIndicator, newShowFormulaIndicator;
    bool oldShowCommentIndicator, newShowCommentIndicator;
    bool oldColumnAsNumber, newColumnAsNumber;
    bool oldLcMode, newLcMode;
    bool oldCapitalizeFirstLetter, newCapitalizeFirstLetter;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_SHEET_COMMANDS