/* This file is part of the KDE project
   Copyright (C) 2001 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KWORD_COMMAND_H
#define KWORD_COMMAND_H

#include <kcommand.h>
#include <koGlobal.h>
#include <qdom.h>
class KWGUI;

namespace Qt3 {
class QTextCommand;
}
class KWTextFrameSet;
class KWDocument;

/**
 * Wraps a QTextCommand into a KCommand, for the UI
 * In fact the QTextCommand isn't even known from here.
 * When the UI orders execute or unexecute, we simply call undo/redo
 * on the KWTextFrameset. Since one KCommand is created for each
 * command there, the two simply map.
 */
class KWTextCommand : public KCommand
{
public:
    KWTextCommand( KWTextFrameSet * textfs, const QString & name ) :
        KCommand( name ), m_textfs(textfs) {}
    ~KWTextCommand() {}

    virtual void execute();
    virtual void unexecute();

protected:
    KWTextFrameSet * m_textfs;
};

////// Extensions to qrichtext_p.h
#include <qrichtext_p.h>
#include <kwtextparag.h>

using namespace Qt3;
typedef QMap<int, QTextCustomItem *> CustomItemsMap;

/**
 * Command created when deleting some text
 */
class KWTextDeleteCommand : public QTextDeleteCommand
{
public:
    KWTextDeleteCommand( QTextDocument *d, int i, int idx, const QArray<QTextStringChar> &str,
                         const CustomItemsMap & customItemsMap,
                         const QValueList<KWParagLayout> & oldParagLayouts );
    //KWTextDeleteCommand( QTextParag *p, int idx, const QArray<QTextStringChar> &str );
    QTextCursor *execute( QTextCursor *c );
    QTextCursor *unexecute( QTextCursor *c );
protected:
    QValueList<KWParagLayout> m_oldParagLayouts;
    CustomItemsMap m_customItemsMap;
};

/**
 * Command created when inserting some text
 */
class KWTextInsertCommand : public KWTextDeleteCommand
{
public:
    KWTextInsertCommand( QTextDocument *d, int i, int idx, const QArray<QTextStringChar> &str,
                         const CustomItemsMap & customItemsMap,
                         const QValueList<KWParagLayout> &oldParagLayouts )
        : KWTextDeleteCommand( d, i, idx, str, customItemsMap, oldParagLayouts ) {}
    //KWTextInsertCommand( QTextParag *p, int idx, const QArray<QTextStringChar> &str )
    //    : KWTextDeleteCommand( p, idx, str ) {}
    Commands type() const { return Insert; };
    QTextCursor *execute( QTextCursor *c ) { return KWTextDeleteCommand::unexecute( c ); }
    QTextCursor *unexecute( QTextCursor *c ) { return KWTextDeleteCommand::execute( c ); }
};

/**
 * Command created when changing paragraph attributes
 */
class KWTextParagCommand : public QTextCommand
{
public:
    enum Flags { All, Alignment, Counter, Margin, LineSpacing, Borders, Tabulator, PageBreaking };
    KWTextParagCommand( QTextDocument *d, int fParag, int lParag,
                        const QValueList<KWParagLayout> &oldParagLayouts,
                        KWParagLayout newParagLayout,
                        Flags flags,
                        QStyleSheetItem::Margin margin = QStyleSheetItem::MarginLeft ); // Only meaningful if flags==Margin
    QTextCursor *execute( QTextCursor *c );
    QTextCursor *unexecute( QTextCursor *c );
protected:
    int firstParag, lastParag;
    QValueList<KWParagLayout> m_oldParagLayouts;
    KWParagLayout m_newParagLayout;
    int m_flags:4;
    int m_margin:3;
};

/**
 * Command created when changing the default format of paragraphs.
 * This is ONLY used for counters and bullet's formatting.
 * See QTextFormatCommand for the command used when changing the formatting of any set of characters.
 */
class KWParagFormatCommand : public QTextCommand
{
public:
    KWParagFormatCommand( QTextDocument *d, int fParag, int lParag,
                          const QValueList<QTextFormat *> &oldFormats,
                          QTextFormat * newFormat );
    ~KWParagFormatCommand();
    QTextCursor *execute( QTextCursor *c );
    QTextCursor *unexecute( QTextCursor *c );
protected:
    int firstParag, lastParag;
    QValueList<QTextFormat *> m_oldFormats;
    QTextFormat * m_newFormat;
};

/**
 * Command created when pasting formatted text
 */
class KWPasteCommand : public QTextCommand
{
public:
    KWPasteCommand( QTextDocument *d, int parag, int idx,
                    const QCString & data );
    ~KWPasteCommand() {}
    QTextCursor *execute( QTextCursor *c );
    QTextCursor *unexecute( QTextCursor *c );
protected:
    int m_parag;
    int m_idx;
    QCString m_data;
    // filled in by execute(), for unexecute()
    int m_lastParag;
    int m_lastIndex;
};

////////////////////////// Frame commands ////////////////////////////////

// Identifies a frame
struct FrameIndex {
    unsigned int m_iFrameSetIndex;
    unsigned int m_iFrameIndex;
};

enum FrameBorderType { FBLeft=0, FBRight=1, FBTop=2, FBBottom=3};

struct FrameBorderTypeStruct {
    FrameBorderType m_EFrameType;
    Border m_OldBorder;
};

/**
 * Command created when changing frame border
 */
class KWFrameBorderCommand : public KCommand
{
public:
    KWFrameBorderCommand( const QString &name,KWDocument *_doc,QList<FrameIndex> &_listFrameIndex, QList<FrameBorderTypeStruct> &_frameTypeBorder,const Border & _newBorder ) ;
    ~ KWFrameBorderCommand() {}

    void execute();
    void unexecute();

protected:
    QList<FrameIndex> m_IndexFrame;
    QList<FrameBorderTypeStruct> m_oldBorderFrameType;
    Border m_newBorder;
    KWDocument *m_pDoc;
};

/**
 * Command created when changing background color of one or more frames
 */
class KWFrameBackGroundColorCommand : public KCommand
{
public:
    KWFrameBackGroundColorCommand( const QString &name,KWDocument *_doc,QList<FrameIndex> &_listFrameIndex, QList<QBrush> &_oldBrush,const QBrush & _newColor ) ;
    ~KWFrameBackGroundColorCommand() {}

    void execute();
    void unexecute();

protected:
    QList<FrameIndex> m_IndexFrame;
    QList<QBrush> m_oldBackGroundColor;
    QBrush m_newColor;
    KWDocument *m_pDoc;
};

struct FrameResizeStruct {
    QRect sizeOfBegin;
    QRect sizeOfEnd;
};

/**
 * Command created when a frame is resized
 */
class KWFrameResizeCommand : public KCommand
{
public:
    KWFrameResizeCommand( const QString &name,KWDocument *_doc,FrameIndex _frameIndex,FrameResizeStruct _frameResize ) ;
    ~KWFrameResizeCommand() {}

    void execute();
    void unexecute();

protected:
    FrameIndex m_IndexFrame;
    FrameResizeStruct m_FrameResize;
    KWDocument *m_pDoc;
};

/**
 * Command created when one or more frames are moved
 */
class KWFrameMoveCommand : public KCommand
{
public:
    KWFrameMoveCommand( const QString &name,KWDocument *_doc,QList<FrameIndex> &_frameIndex,QList<FrameResizeStruct>&_frameMove ) ;
    ~KWFrameMoveCommand() {}

    void execute();
    void unexecute();
    QList<FrameResizeStruct> getListFrameMoved() {return m_frameMove;}
    void setListFrameMoved(QList<FrameResizeStruct>_listMove) {m_frameMove=_listMove;}
protected:
    QList<FrameIndex> m_IndexFrame;
    QList<FrameResizeStruct> m_frameMove;
    KWDocument *m_pDoc;
};


///////////////////////////////layout command///////////////////////////
struct pageLayout {
    KoPageLayout _pgLayout;
    KoColumns _cl;
    KoKWHeaderFooter _hf;
};

/**
 * Command created when you change layout
 */
class KWPageLayoutCommand : public KCommand
{
public:
    KWPageLayoutCommand( const QString &name,KWDocument *_doc,KWGUI *_gui,pageLayout &_oldLayout, pageLayout &_newLayout) ;
    ~KWPageLayoutCommand() {}

    void execute();
    void unexecute();
protected:
    KWDocument *m_pDoc;
    KWGUI *m_pGui;
    pageLayout m_OldLayout;
    pageLayout m_NewLayout;
};


/**
 * Command created when you delete a frame
 */
class KWDeleteFrameCommand : public KCommand
{
public:
    KWDeleteFrameCommand( const QString &name, KWDocument *_doc, KWFrame * frame) ;
    ~KWDeleteFrameCommand() {}

    void execute();
    void unexecute();
protected:
    KWDocument *m_pDoc;
    FrameIndex frameIndex;
    KWFrame *copyFrame;
};


#endif
