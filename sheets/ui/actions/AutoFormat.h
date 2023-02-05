/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998-2023 The Calligra Team <calligra-devel@kde.org>
   SPDX-FileCopyrightText: 2023 Tomas Mecir <mecirt@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_ACTION_AUTOFORMAT
#define CALLIGRA_SHEETS_ACTION_AUTOFORMAT


#include "CellAction.h"

#include "ui/commands/AbstractRegionCommand.h"


class KoXmlDocument;

namespace Calligra
{
namespace Sheets
{
class Style;


/**
 * \ingroup Commands
 * \brief Formats a cell range using a pre-defined style for the table layout.
 */
class AutoFormatCommand : public AbstractRegionCommand
{
public:
    /**
     * Constructor.
     */
    AutoFormatCommand();

    /**
     * Destructor.
     */
    ~AutoFormatCommand() override;

    void setStyles(const QList<Style>& styles);

protected:
    bool process(Element* element) override;

private:
    QList<Style> m_styles;
};



class AutoFormatDialog;

class AutoFormat : public CellAction {
Q_OBJECT
public:
    AutoFormat(Actions *actions);
    virtual ~AutoFormat();
protected:
    virtual void execute(Selection *selection, Sheet *sheet, QWidget *canvasWidget) override;
    QList<Style> parseXML(const KoXmlDocument& doc, bool *ok);
    bool enabledForSelection(Selection *selection, const Cell &) override;
    AutoFormatDialog *m_dlg;
};






} // namespace Sheets
} // namespace Calligra

#endif   // CALLIGRA_SHEETS_ACTION_AUTOFORMAT
