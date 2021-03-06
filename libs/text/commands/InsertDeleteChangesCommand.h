/*
 *  SPDX-FileCopyrightText: 2011 C. Boemann <cbo@boemann.dk>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef INSERTDELETECHANGESCOMMAND_H
#define INSERTDELETECHANGESCOMMAND_H

#include <kundo2command.h>
#include <QPointer>

class QTextDocument;

class InsertDeleteChangesCommand : public KUndo2Command
{
public:
    explicit InsertDeleteChangesCommand(QTextDocument *document, KUndo2Command *parent = 0);
    void redo();

private:
    QPointer<QTextDocument> m_document;
    void insertDeleteChanges();
};


#endif // INSERTDELETECHANGESCOMMAND_H
