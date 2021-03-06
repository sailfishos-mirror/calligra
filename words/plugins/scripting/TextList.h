/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2006 Sebastian Sauer <mail@dipe.org>
 * SPDX-FileCopyrightText: 2010 Thomas Zander <zander@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_TEXTLIST_H
#define SCRIPTING_TEXTLIST_H

#include <QObject>
#include "TextCursor.h"
#include "Style.h"

#include <kdebug.h>

namespace Scripting
{

/**
* The TextList provides a list of items within a \a TextDocument .
*/
class TextList : public QObject
{
    Q_OBJECT
public:
    TextList(QObject* parent, QTextList* list)
            : QObject(parent), m_list(list) {}
    virtual ~TextList() {}

public Q_SLOTS:

    /** Return the number of items the list has. */
    int countItems() {
        return m_list ? m_list.data()->count() : 0;
    }

    /** Return a \a TextCursor object for the item at the position \p index . */
    QObject* item(int index) {
        QTextCursor cursor = m_list ? QTextCursor(m_list.data()->item(index)) : QTextCursor();
        return cursor.isNull() ? 0 : new TextCursor(this, cursor);
    }

#if 0
    QObject* addItem() {
        if (! m_list)
            return 0;
        QTextBlock block;
        m_list->add(block);
        QTextCursor cursor(block);
        return cursor.isNull() ? 0 : new TextCursor(this, cursor);
    }
#endif

    /** Return the content as text of the defined item with index \p index . */
    QString itemText(int index) {
        QTextBlock block = m_list ? m_list.data()->item(index) : QTextBlock();
        return block.isValid() ? m_list.data()->itemText(block) : QString();
    }

    /** Return the content of the list as text. */
    QString text() {
        QString result;
        const int count = m_list ? m_list.data()->count() : 0;
        for (int i = 0; i < count; ++i) {
            const QString s = m_list.data()->itemText(m_list.data()->item(i));
            if (! s.isNull())
                result += QString("%1\n").arg(s);
        }
        return result;
    }

    /** Remove the item at position \p index . */
    void removeItem(int index) {
        if (m_list)
            m_list.data()->removeItem(index);
    }

    /** Set the style this TextList uses to the as argument passed \a Style object. */
    void setStyle(QObject* style) {
        ParagraphStyle* s = dynamic_cast<ParagraphStyle*>(style);
        if (! s) {
            kWarning(32001) << "TextList.setStyle Invalid ParagraphStyle object";
            return;
        }
        KoParagraphStyle* ps = s->style();
        if (! ps) {
            kWarning(32001) << "TextList.setStyle Invalid KoParagraphStyle object";
            return;
        }
        const int count = m_list ? m_list.data()->count() : 0;
        for (int i = 0; i < count; ++i) {
            QTextBlock block = m_list.data()->item(i);
            ps->applyStyle(block);
        }
    }

#if 0
    void setListStyle(int liststyle) {
        KoListStyle s;
        s.setStyle((KoListStyle::Style)liststyle);
        const int count = m_list ? m_list->count() : 0;
        for (int i = 0; i < count; i++)
            s.applyStyle(m_list->item(i));
    }
#endif

private:
    QWeakPointer<QTextList> m_list;
};

}

#endif
