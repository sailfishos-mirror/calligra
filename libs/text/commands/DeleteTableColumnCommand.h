// This file is part of the KDE project
// SPDX-FileCopyrightText: 2009 Pierre Stirnweiss <pstirnweiss@googlemail.com>
// SPDX-FileCopyrightText: 2010 C. Boemann <cbo@boemann.dk>
// SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef DELETETABLECOLUMNCOMMAND_H
#define DELETETABLECOLUMNCOMMAND_H

#include <kundo2command.h>
#include <QVector>
#include <KoTableColumnStyle.h>

class KoTextEditor;
class QTextTable;

class DeleteTableColumnCommand : public KUndo2Command
{
public:

    DeleteTableColumnCommand(KoTextEditor *te, QTextTable *t, KUndo2Command *parent = 0);

    void undo() override;
    void redo() override;

private:
    bool m_first;
    KoTextEditor *m_textEditor;
    QTextTable *m_table;
    int m_selectionColumn;
    int m_selectionColumnSpan;
    QVector<KoTableColumnStyle> m_deletedStyles;
};

#endif // DELETETABLEROWCOMMAND_H
