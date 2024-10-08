/*
 *  SPDX-FileCopyrightText: 2001 Graham Short. <grahshrt@netscape.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <qpro/common.h>

#include <string.h>

#include <iostream>

#include <qpro/record.h>
#include <qpro/formula.h>

// -----------------------------------------------------------------------

#include <iomanip>
#include <sstream>

void
Charout(ostream& pOut, unsigned char pChar)
{
    pOut << ((pChar < 32) || (pChar > 126) ? '.' : (char)pChar);
}

void
Hexout(ostream& pOut, unsigned char pChar)
{
    pOut << setiosflags(ios::uppercase)
    << setfill('0')
    << setw(2)
    << hex
    << (int)pChar
    << dec;
}

int
Hexout(char* pChar, int pLen)
{
    std::ostringstream*   lOStr = new std::ostringstream;

    while (pLen) {
        int lIdx = 0;

        for (lIdx = 0; lIdx < 16; ++lIdx) {
            if (pLen) {
                Hexout(cerr, *pChar);
                cerr << (lIdx == 8 ? "-" : " ");
                Charout(*lOStr, (unsigned char)*pChar);
                ++pChar;
                --pLen;
            } else {
                cerr << "   ";
            }
        }

        cerr << lOStr->rdbuf() << endl;

        delete lOStr;
        lOStr = new std::ostringstream;
    }

    delete lOStr;
    lOStr = nullptr;

    return 0;
}

// -----------------------------------------------------------------------

QpRec::QpRec(QpRecType pType)
        : cType(pType)
{
}

QpRec::~QpRec()
= default;

QP_INT16
QpRec::type()
{
    return cType;
}

// -----------------------------------------------------------------------
//
//
//QpCellRef::QpCellRef(QpIStream& pIn)
//{
//   pIn >> cNoteBook >> cColumn >> cPage >> cRow;
//
//QP_DEBUG("CellRef: NoteBook" << cNoteBook << ", col "
//         << cColumn << ", Page " << (int)cPage << ", Row "
//         << cRow << Qt::endl
//        );
//}
//
//QpCellRef::~QpCellRef()
//{
//}
//
//QP_UINT8
//QpCellRef::column()
//{
//   return cColumn;
//}
//
//QP_INT16
//QpCellRef::row()
//{
//   return cRow;
//}
//
// -----------------------------------------------------------------------

QpRecCell::QpRecCell(QpRecType pType)
        : QpRec(pType)
        , cAttributes(0)
        , cColumn(0)
        , cPage(0)
        , cRow(0)
        , cCellRef(nullptr)
{
}

QpRecCell::~QpRecCell()
{
    delete [] cCellRef;
    cCellRef = nullptr;
}


void
QpRecCell::attributes(QP_INT16 pAttributes)
{
    cAttributes = pAttributes;
}

QP_INT16
QpRecCell::attributes()
{
    return cAttributes;
}

void
QpRecCell::column(QP_UINT8 pColumn)
{
    cColumn = pColumn;
}

QP_UINT8
QpRecCell::column()
{
    return cColumn;
}

void
QpRecCell::row(QP_INT16 pRow)
{
    cRow = pRow;
}

QP_INT16
QpRecCell::row()
{
    return cRow;
}

int
QpRecCell::loadCellInfo(QpIStream& pIn)
{
    pIn >> cColumn >> cPage >> cRow >> cAttributes;

    QP_DEBUG(" col " << (unsigned)cColumn << ", Page " << (unsigned)cPage
             << ", Row " << cRow << ", Ref "
             << /*???cellRef()
            <<*/ ", Attr " << cAttributes
            );

    return 6;  // number of bytes consumed
}

//const char*
//QpRecCell::cellRef()
//{
//  if( cCellRef == 0 )
//  {
//     cCellRef = new char[20]; // hardcoded len???
//
/// ??? what value should notebook param be?
//     cellRef( cCellRef, 0, 0, cColumn, cRow ); //hardcoded page no. ?????
//  }
//
//  return cCellRef;
//

void
QpRecCell::cellRef(char* pText, QpTableNames& pTable, QP_INT16 /*pNoteBook*/, QP_UINT8 pPage, QP_UINT8 pColumn, QP_INT16 pRow)
{
//??? cope with relative/absolute references

    std::ostringstream lOut; 
    int       lPageRelative = pRow & 0x8000;
    int       lColRelative  = pRow & 0x4000;
    int       lRowRelative  = pRow & 0x2000;
    QP_UINT8  lCol          = (lColRelative ? cColumn + pColumn : pColumn);

    // Sign bit for row is in bit 0x1000, so either set all top bits or lose all top bits
    QP_INT16  lRow = (lRowRelative ? cRow + (pRow & 0x1000 ? pRow | 0xE000 : pRow & 0x1FFF)
                              : pRow & 0x1FFF
                             );

    // Are we referencing a different page ?

    if (lPageRelative && (pPage == 0)) {
        // no - page is zero relative to this one
    } else if (pPage != cPage) {
        // yes - not relative & page is a different one

        QP_UINT8 lPage = (lPageRelative ? pPage + cPage : pPage);

        QP_DEBUG("pTable.name((unsigned)lPage) = " <<  pTable.name((unsigned)lPage) << Qt::endl);

        lOut << pTable.name((unsigned)lPage) << '!'; // is '!' compat with QPRO???
    }

    if (!lColRelative) {
        lOut << '$';
    }
    if (lCol < 26) {
        lOut << (char)('A' + lCol);
    } else {
        lOut << (char)('A' - 1 + lCol / 26)
        << (char)('A' + lCol % 26);
    }

    if (!lRowRelative) {
        lOut << '$';
    }

    lOut << (lRow & 0x1FFF) + 1;
    strncpy(pText, lOut.str().c_str(), 20);// ??? ard coded len
}

void
QpRecCell::cellRef(char* pText, QpTableNames& pTable, QpIStream& pFormulaRef)
{
    QP_INT16 lNoteBook;
    pFormulaRef >> lNoteBook;

    // block references (eg. A1..A9) have bit 0x1000 set

    if (lNoteBook & 0x1000) {
        QP_UINT8 lFirstColumn;
        QP_UINT8 lFirstPage;
        QP_INT16 lFirstRow;
        QP_UINT8 lLastColumn;
        QP_UINT8 lLastPage;
        QP_INT16 lLastRow;

        pFormulaRef >> lFirstColumn
        >> lFirstPage
        >> lFirstRow
        >> lLastColumn
        >> lLastPage
        >> lLastRow;

        QP_DEBUG("BlockRef: NoteBook " << lNoteBook
                 << ", 1st col " << lFirstColumn
                 << ", 1st page " << (unsigned)lFirstPage
                 << ", 1st row " << lFirstRow
                 << ", last col " << lLastColumn
                 << ", last page " << (unsigned)lLastPage
                 << ", last row " << lLastRow
                 << Qt::endl
                );
// ??? next few lines shouldn't just add rows together
        cellRef(pText, pTable, lNoteBook, lFirstPage, lFirstColumn, lFirstRow);
// ?? temp next line      strcat( pText, ".." );
        strcat(pText, ":");
        cellRef(&pText[strlen(pText)], pTable, lNoteBook, lLastPage, lLastColumn, lLastRow);
    } else {
        QP_UINT8 lColumn;
        QP_UINT8 lPage;
        QP_INT16 lRow;

        pFormulaRef >> lColumn >> lPage >> lRow;

        QP_DEBUG("FormulaRef: NoteBook " << lNoteBook << ", Col " << (unsigned)lColumn
                 << ", Page " << (unsigned)lPage << ", Row " << lRow << Qt::endl
                );

// ??? sort out what to do about lNotebook
// ??? next few lines shouldn't just add rows together
        cellRef(pText, pTable, lNoteBook, lPage, lColumn, lRow);
    }
}

// -----------------------------------------------------------------------

QpRecBof::QpRecBof(QP_INT16, QpIStream& pIn)
        : QpRec(QpBof)
{
    pIn >> cFileFormat;

    QP_DEBUG("BOF fileformat=" << cFileFormat << Qt::endl);
}

QpRecBof::~QpRecBof()
= default;

// -----------------------------------------------------------------------

QpRecEof::QpRecEof(QP_INT16, QpIStream&)
        : QpRec(QpEof)
{
    QP_DEBUG("EOF" << Qt::endl);
}

QpRecEof::~QpRecEof()
= default;


// -----------------------------------------------------------------------

QpRecRecalcMode::QpRecRecalcMode(QP_INT16, QpIStream& pIn)
        : QpRec(QpRecalcMode)
{
    QP_INT8 lMode;

    pIn >> lMode;

    cMode = (MODE)(unsigned char) lMode;

    QP_DEBUG("Recalc Mode = "
             << (int)lMode << (cMode == Manual ? " (Manual)"
                               : cMode == Background ? " (Background)"
                               : cMode == Automatic  ? " (Automatic)"
                               : " (Unknown)"
                              )
             << Qt::endl
            );
}

QpRecRecalcMode::~QpRecRecalcMode()
= default;


void
QpRecRecalcMode::mode(MODE pMode)
{
    cMode = pMode;
}

QpRecRecalcMode::MODE
QpRecRecalcMode::mode()
{
    return cMode;
}


// -----------------------------------------------------------------------

QpRecRecalcOrder::QpRecRecalcOrder(QP_INT16, QpIStream& pIn)
        : QpRec(QpRecalcOrder)
{
    QP_INT8 lOrder;

    pIn >> lOrder;

    cOrder = (ORDER)(unsigned char) lOrder;

    QP_DEBUG("Recalc Order = "
             << (int)lOrder << (cOrder == Natural ? " (Natural)"
                                : cOrder == Column ? " (Column)"
                                : cOrder == Row  ? " (Row)"
                                : " (Unknown)"
                               )
             << Qt::endl
            );
}

QpRecRecalcOrder::~QpRecRecalcOrder()
= default;


void
QpRecRecalcOrder::order(ORDER pOrder)
{
    cOrder = pOrder;
}

QpRecRecalcOrder::ORDER
QpRecRecalcOrder::order()
{
    return cOrder;
}


// -----------------------------------------------------------------------

QpRecEmptyCell::QpRecEmptyCell(QP_INT16, QpIStream& pIn)
        : QpRecCell(QpEmptyCell)
{
    QP_DEBUG("Empty Cell - ");

    loadCellInfo(pIn);

    QP_DEBUG(Qt::endl);
}

QpRecEmptyCell::~QpRecEmptyCell()
= default;


// -----------------------------------------------------------------------

QpRecIntegerCell::QpRecIntegerCell(QP_INT16, QpIStream& pIn)
        : QpRecCell(QpIntegerCell)
{
    QP_DEBUG("Integer Cell - ");

    loadCellInfo(pIn);

    pIn >> cInt;

    QP_DEBUG(", Int " << cInt << Qt::endl);
}

QpRecIntegerCell::~QpRecIntegerCell()
= default;

QP_INT16
QpRecIntegerCell::integer()
{
    return cInt;
}

// -----------------------------------------------------------------------

QpRecFloatingPointCell::QpRecFloatingPointCell(QP_INT16, QpIStream& pIn)
        : QpRecCell(QpFloatingPointCell)
{
    QP_DEBUG("Float Cell - ");

    loadCellInfo(pIn);

    pIn >> cValue;

    QP_DEBUG(", Value " << cValue << Qt::endl);
}

QpRecFloatingPointCell::~QpRecFloatingPointCell()
= default;

QP_INT64
QpRecFloatingPointCell::value()
{
    return cValue;
}

// -----------------------------------------------------------------------

QpRecLabelCell::QpRecLabelCell(QP_INT16 pLen, QpIStream& pIn)
        : QpRecCell(QpLabelCell)
{
    QP_DEBUG("Label Cell - ");
    int lLabelLen = pLen - loadCellInfo(pIn) - 1;

    pIn >> cLabelPrefix;

    cLabel = new char[lLabelLen];

    pIn.read(cLabel, lLabelLen);

    QP_DEBUG(", Prefix " << cLabelPrefix << ", Label " << cLabel << Qt::endl);
}

QpRecLabelCell::~QpRecLabelCell()
{
    delete [] cLabel;
    cLabel =  nullptr;
}

char
QpRecLabelCell::labelPrefix()
{
    return cLabelPrefix;
}

const char*
QpRecLabelCell::label()
{
    return cLabel;
}

// -----------------------------------------------------------------------

QpRecFormulaCell::QpRecFormulaCell(QP_INT16 pLen, QpIStream& pIn)
        : QpRecCell(QpFormulaCell)
        , cFormula(nullptr)
{
    QP_DEBUG("Formula Cell - ");

    int lFormulaLen = pLen - loadCellInfo(pIn);

    pIn >> cLastValue;
    lFormulaLen -= 8;

    pIn >> cState;
    lFormulaLen -= 2;

    pIn >> cLen;
    lFormulaLen -= 2;

    pIn >> cCellRef;
    lFormulaLen -= 2;

    cFormula = new char[lFormulaLen];

    pIn.read(cFormula, lFormulaLen);

    QP_DEBUG(", LastValue " << cLastValue << ", State " << cState << Qt::endl);
    QP_DEBUG("   FormulaLen " << cLen << ", CellRef " << cCellRef << ", Formula" << Qt::endl);
#ifdef QP_TRACE
    Hexout(cFormula, lFormulaLen);
#endif
    QP_DEBUG(Qt::endl);
}

QpRecFormulaCell::~QpRecFormulaCell()
{
    delete [] cFormula;
    cFormula = nullptr;
}

const char*
QpRecFormulaCell::formula()
{
    return cFormula;
}

QP_INT16
QpRecFormulaCell::formulaLen()
{
    return cLen;
}

QP_INT16
QpRecFormulaCell::formulaReferences()
{
    return cCellRef;
}

// -----------------------------------------------------------------------

QpRecUnknown::QpRecUnknown(QP_INT16 /*pType*/, QP_INT16 pLen, QpIStream& pIn)
        : QpRec(QpUnknown)
{
    QP_DEBUG("Unknown Type " << pType << ", len " << pLen << Qt::endl);

    if (pLen > 0) {
        char* lBuf = new char[pLen];

        pIn.read(lBuf, pLen);

        delete [] lBuf;
        lBuf = nullptr;
    }
}

QpRecUnknown::~QpRecUnknown()
= default;

// -----------------------------------------------------------------------

QpRecBop::QpRecBop(QP_INT16, QpIStream& pIn)
        : QpRec(QpBop)
{
    pIn >> cPageIndex;
    QP_DEBUG("BOP: " << (unsigned)cPageIndex << Qt::endl);
}

QpRecBop::~QpRecBop()
= default;


QP_UINT8
QpRecBop::pageIndex()
{
    return cPageIndex;
}


// -----------------------------------------------------------------------

QpRecPageName::QpRecPageName(QP_INT16, QpIStream& pIn)
        : QpRec(QpPageName)
{
    pIn >> cPageName;

    QP_DEBUG("Page Name: " << cPageName << Qt::endl);
}

QpRecPageName::~QpRecPageName()
{
    delete [] cPageName;
}

const char*
QpRecPageName::pageName()
{
    return cPageName;
}
// -----------------------------------------------------------------------

QpRecPassword::QpRecPassword(QP_INT16 pLen, QpIStream& pIn)
        : QpRec(QpPassword)
{
    QP_DEBUG("Password len = " << pLen << Qt::endl);

    cPassword = new QP_UINT8[pLen];

    pIn.read((char*)cPassword, pLen);

    QP_DEBUG("Password(Hex) = ");
#ifdef QP_TRACE
    Hexout((char*)cPassword, pLen);
#endif
    QP_DEBUG(Qt::endl);
}

QpRecPassword::~QpRecPassword()
{
    delete [] cPassword;
    cPassword = nullptr;
}

const QP_UINT8*
QpRecPassword::password()
{
    return cPassword;
}

