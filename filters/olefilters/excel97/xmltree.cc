/* This file is part of the KDE project

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

#include <xmltree.h>
#include <qstringlist.h>
#include <kdebug.h>

const char *palette[65] = {
  "#000000", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff",
  "#ffffff", "#ffffff", "#000000", "#ffffff", "#ff0000", "#00ff00",
  "#0000ff", "#ffff00", "#ff00ff", "#00ffff", "#800000", "#008000",
  "#000080", "#808000", "#800080", "#008080", "#c0c0c0", "#808080",
  "#9999ff", "#993366", "#ffffcc", "#ccffff", "#660066", "#ff8080",
  "#0066cc", "#ccccff", "#000080", "#ff00ff", "#ffff00", "#00ffff",
  "#800080", "#800000", "#008080", "#0000ff", "#00ccff", "#ccffff",
  "#ccffcc", "#ffff99", "#99ccff", "#ff99cc", "#cc99ff", "#ffcc99",
  "#3366ff", "#33cccc", "#99cc00", "#ffcc00", "#ff9900", "#ff6600",
  "#666699", "#969696", "#003366", "#339966", "#003300", "#333300",
  "#993300", "#993366", "#333399", "#333333", "#ffffff"
};

const int ndays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int ldays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

XMLTree::XMLTree():QObject(),table(0L), fontCount(0), footerCount(0),
                          headerCount(0), xfCount(0)
{
  QDomElement e;

  root = new QDomDocument("spreadsheet");

  root->appendChild(root->createProcessingInstruction
                    ("xml", "version=\"1.0\" encoding =\"UTF-8\""));

  doc = root->createElement("spreadsheet");
  doc.setAttribute("author", "OLEFilter");
  doc.setAttribute("email", "unknown");
  doc.setAttribute("editor", "KSpread");
  doc.setAttribute("mime", "application/x-kspread");
  root->appendChild(doc);

  paper = root->createElement("paper");
  paper.setAttribute("format", "A4");
  paper.setAttribute("orientation", "Portrait");
  doc.appendChild(paper);

  e = root->createElement("borders");
  e.setAttribute("left", 20);
  e.setAttribute("top", 20);
  e.setAttribute("right", 20);
  e.setAttribute("bottom", 20);
  paper.appendChild(e);

  map = root->createElement("map");
  doc.appendChild(map);
}

XMLTree::~XMLTree()
{
  delete root;
  root=0L;
}

const QDomDocument* const XMLTree::part()
{
  return root;
}

void XMLTree::getFont(Q_UINT16 /*xf*/, QDomElement &f, Q_UINT16 fontid)
{
  QDomElement font = root->createElement("font");

  font.setAttribute("family", fonts[fontid]->rgch);
  font.setAttribute("size", (fonts[fontid]->dyHeight / 20) + 2);
  font.setAttribute("weight", fonts[fontid]->bls / 8);

  if ((fonts[fontid]->bls / 8) != 50)
    font.setAttribute("bold", "yes");

  if ((fonts[fontid]->grbit & 0x02) == 2)
    font.setAttribute("italic", "yes");

  if (fonts[fontid]->uls != 0)
    font.setAttribute("underline", "yes");

  f.appendChild(font);
}

void XMLTree::getPen(Q_UINT16 xf, QDomElement &f, Q_UINT16 fontid)
{
  QDomElement pen = root->createElement("pen");

  pen.setAttribute("width", 0);
  pen.setAttribute("style", 1);
  pen.setAttribute("color", palette[(fonts[fontid]->icv) & 0x7f]);
  f.appendChild(pen);

  if (xfs[xf]->borderStyle & 0x0f != 0) {
    QDomElement leftBorder = root->createElement("left-border");
    pen = root->createElement("pen");
    pen.setAttribute("width", 2);
    pen.setAttribute("style", xfs[xf]->borderStyle & 0x0f);
    pen.setAttribute("color", palette[xfs[xf]->sideBColor & 0x7f]);
    leftBorder.appendChild(pen);
    f.appendChild(leftBorder);
  }

  if ((xfs[xf]->borderStyle >> 4) & 0x0f != 0) {
    QDomElement topBorder = root->createElement("right-border");
    pen = root->createElement("pen");
    pen.setAttribute("width", 2);
    pen.setAttribute("style", (xfs[xf]->borderStyle >> 4) & 0x0f);
    pen.setAttribute("color", palette[(xfs[xf]->sideBColor >> 7) & 0x7f]);
    topBorder.appendChild(pen);
    f.appendChild(topBorder);
  }

  if ((xfs[xf]->borderStyle >> 8) & 0x0f != 0) {
    QDomElement topBorder = root->createElement("top-border");
    pen = root->createElement("pen");
    pen.setAttribute("width", 2);
    pen.setAttribute("style", (xfs[xf]->borderStyle >> 8) & 0x0f);
    pen.setAttribute("color", palette[xfs[xf]->topBColor & 0x7f]);
    topBorder.appendChild(pen);
    f.appendChild(topBorder);
  }

  if ((xfs[xf]->borderStyle >> 12) & 0x0f != 0) {
    QDomElement topBorder = root->createElement("bottom-border");
    pen = root->createElement("pen");
    pen.setAttribute("width", 2);
    pen.setAttribute("style", (xfs[xf]->borderStyle >> 12) & 0x0f);
    pen.setAttribute("color", palette[(xfs[xf]->topBColor >> 7) & 0x7f]);
    topBorder.appendChild(pen);
    f.appendChild(topBorder);
  }
}

void XMLTree::getDate(int date, int& year, int& month, int& day)
{
  year = 0;

  while (date > (((year % 4) == 0) ? 366 : 365))
    date -= ((year++%4) == 0) ? 366 : 365;
		
  if (date1904 == 1)
    year += 4;		

  if ((year % 4) == 0) {	
      for (month = 0; month < 12; ++month) {
	if (date <= ldays[month])
	  break;
	date -= ldays[month];
      }
  }
  else {
    for (month = 0; month < 12; ++month) {
      if (date <= ndays[month])
	break;
      date -= ndays[month];
    }
  }

  ++month;
  day = (date == 0) ? 1 : date;
  year += 1900;
}

const QDomElement XMLTree::getFormat(Q_UINT16 xf)
{
  QString s;
  QDomElement format = root->createElement("format");

  Q_UINT16 fontid = xfs[xf]->ifnt;

  if (fontid > 3)
    --fontid;

  format.setAttribute("bgcolor", palette[xfs[xf]->cellColor & 0x7f]);

  format.setAttribute("align", (xfs[xf]->align & 0x07) == 0 ? 4 : xfs[xf]->align & 0x07);
  format.setAttribute("alignY", ((xfs[xf]->align >> 4) & 0x07) == 3 ? 2 : ((xfs[xf]->align >> 4) & 0x07) + 1);

  int angle = xfs[xf]->align >> 8;
  format.setAttribute("angle", angle < 91 ? angle * (-1) : angle - 90);

  if ((xfs[xf]->align >> 3) & 0x01 == 1)
    format.setAttribute("multirow", "yes");

  switch (xfs[xf]->ifmt) {
  case 0x00:  // We need this to avoid 'default'
    break;
  case 0x01:  // Number 0
    format.setAttribute("precision", "0");
    break;
  case 0x02:  // Number       0.00
    format.setAttribute("precision", "2");
    break;
  case 0x03:  // Number w/comma       0,000
    format.setAttribute("precision", "0");
    break;
  case 0x04:  // Number w/comma       0,000.00
    format.setAttribute("precision", "2");
    break;
  case 0x05:  // Number currency
  case 0x06:
  case 0x07:
  case 0x08:
    format.setAttribute("precision", "2");
    format.setAttribute("format", "10");
    break;
  case 0x09:  // Percent 0%
    format.setAttribute("precision", "0");
    format.setAttribute("format", "25");
    format.setAttribute("faktor", "100");
    break;
  case 0x0A:  // Percent 0.00%
    format.setAttribute("precision", "2");
    format.setAttribute("format", "25");
    format.setAttribute("faktor", "100");
    break;
  case 0x0B:  // Scientific 0.00+E00
    format.setAttribute("precision", "2");
    format.setAttribute("format", "30");
    break;
  case 0x0C:  // Fraction 1 number  e.g. 1/2, 1/3
    format.setAttribute("precision", "-1");
    format.setAttribute("format", "0");
    break;
  case 0x0D:  // Fraction 2 numbers  e.g. 1/50, 25/33
    format.setAttribute("precision", "-1");
    format.setAttribute("format", "0");
    break;
  case 0x0E:	// Date
    format.setAttribute("format", "35");
    break;
  case 0x0F:	// Date
    format.setAttribute("format", "200");
    break;
  case 0x10:	// Date
    format.setAttribute("format", "35");
    break;
  case 0x11:	// Date
    format.setAttribute("format", "206");
    break;
  case 0x12:  // Time: h:mm AM/PM
    format.setAttribute("format", "50");
    break;
  case 0x13:  // Time: h:mm:ss AM/PM
    format.setAttribute("format", "51");
    break;
  case 0x14:  // Time: h:mm
    format.setAttribute("format", "50");
    break;
  case 0x15:  // Time: h:mm:ss
    format.setAttribute("format", "51");
    break;
  case 0x2a:  // Number currency
  case 0x2c:
    format.setAttribute("precision", "2");
    format.setAttribute("format", "10");
    format.setAttribute("faktor", "1");
    break;
  case 0x2D:  // Time: mm:ss
    format.setAttribute("format", "51");
    break;
  case 0x2E:  // Time: [h]:mm:ss
    format.setAttribute("format", "51");
    break;
  case 0x2F:  // Time: mm:ss.0
    format.setAttribute("format", "51");
    break;
  case 0xA4:	// Date
    format.setAttribute("format", "215");
    break;
  case 0xA5:	// Date
    format.setAttribute("format", "204");
    break;
  case 0xA6:	// Date
    format.setAttribute("format", "200");
    break;
  case 0xA7:	// Date
    format.setAttribute("format", "212");
    break;
  case 0xA8:	// Date
    format.setAttribute("format", "203");
    break;
  case 0xA9:	// Date
    format.setAttribute("format", "202");
    break;
  case 0xAA:	// Date
    format.setAttribute("format", "207");
    break;
  case 0xAB:	// Date
    format.setAttribute("format", "209");
    break;
  default:    
    s = QString::fromLatin1(formats[xfs[xf]->ifmt]->rgch,
			    formats[xfs[xf]->ifmt]->cch);
  }

  getFont(xf, format, fontid);
  getPen(xf, format, fontid);

  return format;
}

const QString XMLTree::getFormula(Q_UINT16 row, Q_UINT16 column, QDataStream& rgce)
{
  double number;
  Q_UINT8 byte, ptg;
  Q_UINT16 integer;
  Q_INT16 refRow, refColumn, refRowLast, refColumnLast;
  QString str;
  QStringList parsedFormula;
  QStringList::Iterator stringPtr;

  parsedFormula.append("=");

  while (!rgce.eof()) {
    rgce >> ptg;
    switch (ptg) {
    case 0x03:  // ptgAdd
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "+";
      break;
    case 0x04:  // ptgSub
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "-";
      break;
    case 0x05:  // ptgMul
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "*";
      break;
    case 0x06:  // ptgDiv
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "/";
      break;
    case 0x07:  // ptgPower
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "^"; // Hmmm, not supported by kspread.
      break;
    case 0x08:  // ptgConcat
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "&";
      break;
    case 0x09:  // ptgLT
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "<";
      break;
    case 0x0a:  // ptgLE
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "<=";
      break;
    case 0x0b:  // ptgEQ
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "==";
      break;
    case 0x0c:  // ptgGE
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = ">=";
      break;
    case 0x0d:  // ptgGT
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = ">";
      break;
    case 0x0e:  // ptgNE
      stringPtr = parsedFormula.fromLast();
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty());
      *stringPtr = "!=";
      break;
    case 0x15:  // ptgParen
      stringPtr = parsedFormula.fromLast();
      --stringPtr;
      *stringPtr += ")";
      ++stringPtr;
      do {
	--stringPtr;
	--stringPtr;
      } while (!(*stringPtr).isEmpty() && (*stringPtr) != "=");
      ++stringPtr;
      (*stringPtr).prepend("(");
      break;
    case 0x19:  // ptgAttr
      rgce >> byte >> integer;
      if (byte & 0x10) {
	stringPtr = parsedFormula.fromLast();
	--stringPtr;
	*stringPtr += ")";
	(*stringPtr).prepend("sum(");
	}
      break;
    case 0x1d:  // ptgBool
      rgce >> byte;
      if (byte == 1)
	parsedFormula.append("True");
      else
	parsedFormula.append("False");
      parsedFormula.append("");
      break;
    case 0x1e:  // ptgInt
      rgce >> integer;
      parsedFormula.append(QString::number((int) integer));
      parsedFormula.append("");
      break;
    case 0x1f:  // ptgNum
      rgce >> number;
      parsedFormula.append(QString::number(number));
      parsedFormula.append("");
      break;
    case 0x21:  // ptgFunc
    case 0x41:
      rgce >> integer;
      stringPtr = parsedFormula.fromLast();
      --stringPtr;
      *stringPtr += ")";

      switch (integer) {
      case 15:  // sin
	(*stringPtr).prepend("sin(");
	break;
      case 16:  // cos
	(*stringPtr).prepend("cos(");
	break;
      case 17:  // tan
	(*stringPtr).prepend("tan(");
	break;
      case 18:  // atan
	(*stringPtr).prepend("atan(");
	break;
      case 19:  // pi
	parsedFormula.append("PI()");
	parsedFormula.append("");
	break;
      case 20:  // sqrt
	(*stringPtr).prepend("sqrt(");
	break;
      case 21:  // exp
	(*stringPtr).prepend("exp(");
	break;
      case 22:  // ln
	(*stringPtr).prepend("ln(");
	break;
      case 23:  // log
	(*stringPtr).prepend("log(");
	break;
      case 24:  // fabs
	(*stringPtr).prepend("fabs(");
	break;
      case 25:  // floor
	(*stringPtr).prepend("floor(");
	break;
      case 26:  // sign
	(*stringPtr).prepend("sign(");
	break;
      case 39:  // mod
	--stringPtr;
	*stringPtr = ",";
	--stringPtr;
	(*stringPtr).prepend("MOD(");
	break;
      case 97:  // atan2
	--stringPtr;
	*stringPtr = ",";
	--stringPtr;
	(*stringPtr).prepend("atan2(");
	break;
      case 98:  // asin
	(*stringPtr).prepend("asin(");
	break;
      case 99:  // acos
	(*stringPtr).prepend("acos(");
	break;
      case 184:  // fact
	(*stringPtr).prepend("fact(");
	break;
      case 212:  // ceil
	*stringPtr = ""; // no exact match, so we have to workaround
	--stringPtr;
	*stringPtr = "";
	--stringPtr;
	*stringPtr += ")";
	(*stringPtr).prepend("ceil(");
	break;
      case 229:  // sinh
	(*stringPtr).prepend("sinh(");
	break;
      case 230:  // cosh
	(*stringPtr).prepend("cosh(");
	break;
      case 231:  // tanh
	(*stringPtr).prepend("tanh(");
	break;
      case 232:  // asinh
	(*stringPtr).prepend("asinh(");
	break;
      case 233:  // acosh
	(*stringPtr).prepend("acosh(");
	break;
      case 337:  // pow
	--stringPtr;
	*stringPtr = ",";
	--stringPtr;
	(*stringPtr).prepend("pow(");
	break;
      case 342:  // radian
	(*stringPtr).prepend("radian(");
	break;
      case 343:  // degree
	(*stringPtr).prepend("degree(");
	break;
      default:
	kdDebug(30511) << "Formula contains unhandled function " << integer << endl;
	break;
      }
      break;
    case 0x22:  // ptgFuncVar
    case 0x42:
      rgce >> byte >> integer;
      stringPtr = parsedFormula.fromLast();
      --stringPtr;
      *stringPtr += ")";
      ++stringPtr;
      for (; byte > 1; --byte) {
	--stringPtr;
	--stringPtr;
	*stringPtr = ",";
      } 
      --stringPtr;

      switch (integer) {
      case 4:  // sum
	(*stringPtr).prepend("sum(");
	break;
      case 5:  // average
	(*stringPtr).prepend("average(");
	break;
      case 6:  // min
	(*stringPtr).prepend("min(");
	break;
      case 7:  // max
	(*stringPtr).prepend("max(");
	break;
      case 183:  // multiply
	(*stringPtr).prepend("multiply(");
	break;
      case 197:  // ent
	(*stringPtr).prepend("ENT(");
	*stringPtr += ")"; // no exact match, so we have to workaround
	++stringPtr;
	*stringPtr = ""; 
	++stringPtr;
	*stringPtr = "";
	break;
      default:
	kdDebug(30511) << "Formula contains unhandled function " << integer << endl;
	break;
      }
      break;
    case 0x24:  // ptgRef
    case 0x44:
      if (biff == BIFF_8) { 
	rgce >> refRow >> refColumn;
	if (refColumn & 0x8000)
	  refRow += -1*row;
	if (refColumn & 0x4000) {
	  refColumn &= 0x3fff;
	  refColumn += -1*column;
	}
      } else {
	rgce >> refRow >> byte;
	refColumn = byte; 
	if (refRow & 0x8000)
	  refRow += -1*row;
	if (refRow & 0x4000) {
	  refRow &= 0x3fff;
	  refColumn += -1*column;
	}
      }
      str = "#";
      str += QString::number((int) refColumn);
      str += "#";
      str += QString::number((int) refRow);
      str += "#";
      parsedFormula.append(str);
      parsedFormula.append("");
      break;
    case 0x25:  // ptgArea
    case 0x45:
      if (biff == BIFF_8) { 
	rgce >> refRow >> refRowLast >> refColumn >> refColumnLast;
	if (refColumn & 0x8000)
	  refRow += -1*row;
	if (refColumn & 0x4000) {
	  refColumn &= 0x3fff;
	  refColumn += -1*column;
	}
	if (refColumnLast & 0x8000)
	  refRowLast += -1*row;
	if (refColumnLast & 0x4000) {
	  refColumnLast &= 0x3fff;
	  refColumnLast += -1*column;
	}
      } else {
	rgce >> refRow >> refRowLast >> byte;
	refColumn = byte;
	rgce >> byte;
	refColumnLast = byte;
	if (refRow & 0x8000)
	  refRow += -1*row;
	if (refRow & 0x4000) {
	  refColumn &= 0x3fff;
	  refColumn += -1*column;
	}
	if (refRowLast & 0x8000)
	  refRowLast += -1*row;
	if (refRowLast & 0x4000) {
	  refColumnLast &= 0x3fff;
	  refColumnLast += -1*column;
	}
      }
      str = "#";
      str += QString::number((int) refColumn);
      str += "#";
      str += QString::number((int) refRow);
      str += "#:#";
      str += QString::number((int) refColumnLast);
      str += "#";
      str += QString::number((int) refRowLast);
      str += "#";
      parsedFormula.append(str);
      parsedFormula.append("");
      break;
    default:
      kdDebug(30511) << "Formula contains unhandled ptg " << ptg << endl;
      break;
    }
  }
  kdDebug(30511) << "Formula: " << parsedFormula.join("") << endl;
  return parsedFormula.join("");
}

bool XMLTree::_1904(Q_UINT16 size, QDataStream& body)
{
  body >> date1904;

  return true;
}

bool XMLTree::_array(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_backup(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_blank(Q_UINT16, QDataStream& body)
{
  Q_UINT16 row, column, xf;

  body >> row >> column >> xf;
  QDomElement e = root->createElement("cell");
  e.appendChild(getFormat(xf));
  e.setAttribute("row", (int) ++row);
  e.setAttribute("column", (int) ++column);
  table->appendChild(e);
  return true;
}

bool XMLTree::_bof(Q_UINT16, QDataStream& body)
{
  Q_UINT16 type;

  body >> biff >> type;

  if (biff != BIFF_5_7 && biff != BIFF_8)
    return false;
  if (type == 0x10) {
    if (table != 0L)
      delete table;
    table = tables.dequeue();
  }
  return true;
}

bool XMLTree::_bookbool(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_boolerr(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_bottommargin(Q_UINT16, QDataStream& body)
{
  double value;
  body >> value;
  (paper.namedItem("borders")).toElement().setAttribute("bottom", value);

  return true;
}

bool XMLTree::_boundsheet(Q_UINT16 /*size*/, QDataStream& body)
{
  QDomElement *e;

  if (biff == BIFF_5_7) {
    Q_UINT8 length;
    Q_UINT16 type;
    Q_UINT32 skip;
    body >> skip >> type >> length;
    if ((type & 0x0f) == 0) {
      char *name = new char[length];
      body.readRawBytes(name, length);
      QString s = QString::fromLatin1(name, length);

      delete []name;

      e = new QDomElement(root->createElement("table"));
      e->setAttribute("name", s);
      map.appendChild(*e);
      tables.enqueue(e);
    }
  }
  else if (biff == BIFF_8) {
    Q_UINT16 type, length;
    Q_UINT32 skip;
    body >> skip >> type >> length;
    if ((type & 0x0f) == 0) {
      char *name = new char[length];
      body.readRawBytes(name, length);
      QString s = QString::fromLatin1(name, length);

      delete []name;
      e = new QDomElement(root->createElement("table"));
      e->setAttribute("name", s);
      map.appendChild(*e);
      tables.enqueue(e);
    }
  }
  return true;
}

bool XMLTree::_cf(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_condfmt(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_codepage(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_colinfo(Q_UINT16, QDataStream& body)
{
  Q_UINT16 first, last, width;

  body >> first >> last >> width;

  for (Q_UINT16 i = first; i <= last; ++i) {
    QDomElement col = root->createElement("column");
    col.setAttribute("column", (int) i+1);
    col.setAttribute("width", (int) width / 120);
    table->appendChild(col);
  }

  return true;
}

bool XMLTree::_cont(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_country(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_crn(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_dbcell(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_defaultrowheight(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_defcolwidth(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_dimensions(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_eof(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_externcount(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_externname(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_externsheet(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_extsst(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_filepass(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_filesharing(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_filesharing2(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_font(Q_UINT16 /*size*/, QDataStream& body)
{
  QChar *c;
  Q_UINT8 lsb, msb;

  font_rec *f = new font_rec;
  body >> f->dyHeight >> f->grbit >> f->icv >> f->bls >> f->sss;
  body >> f->uls >> f->bFamily >> f->bCharSet >> f->reserved >> f->cch;
  if (biff == BIFF_5_7) {
    for (int i = 0; i < f->cch; i++) {
      body >> lsb;
      c = new QChar(lsb, 0);
      f->rgch += *c;
    }
  }
  else if (biff == BIFF_8) {
    body >> lsb;
    for (int i = 0; i < f->cch; i++) {
      body >> lsb >> msb;
      c = new QChar(lsb, msb);
      f->rgch += *c;
    }
  }
  fonts.insert(fontCount++, f);

  return true;
}

bool XMLTree::_footer(Q_UINT16 /*size*/, QDataStream& body)
{
  if (footerCount++ == 0) {
    QDomElement e;
    Q_UINT8 length;
    body >> length;
    char *name = new char[length];
    body.readRawBytes(name, length);
    QString s = QString::fromLatin1(name, length);

    e = root->createElement("foot");
    e.setAttribute("left", "");
    //e.setAttribute("center", s);  // BUG?!? (Werner)
    e.setAttribute("center", "");   // replaced by this line :)
    e.setAttribute("right", "");
    paper.appendChild(e);
  }
  return true;
}

bool XMLTree::_format(Q_UINT16 /*size*/, QDataStream& body)
{
  Q_UINT16 id;
  format_rec *f = new format_rec;
  body >> id >> f->cch;
  f->rgch = new char[f->cch];
  body.readRawBytes(f->rgch, f->cch);
  formats.insert(id, f);

  return true;
}

bool XMLTree::_formula(Q_UINT16 /*size*/, QDataStream& body)
{
  Q_UINT16 row, column, xf, skip;

  body >> row >> column >> xf;
  body >> skip >> skip >> skip >> skip >> skip >> skip >> skip >> skip;

  QDomElement e = root->createElement("cell");
  e.appendChild(getFormat(xf));
  e.setAttribute("row", (int) row+1);
  e.setAttribute("column", (int) column+1);

  QDomElement text = root->createElement("text");
  text.appendChild(root->createTextNode(getFormula(row, column, body)));
  e.appendChild(text);
  table->appendChild(e);
  
  return true;
}

bool XMLTree::_gcw(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_guts(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_hcenter(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_header(Q_UINT16 /*size*/, QDataStream& body)
{
  if (headerCount++ == 0) {
    QDomElement e;
    Q_UINT8 length;
    body >> length;
    char *name = new char[length];
    body.readRawBytes(name, length);
    QString s = QString::fromLatin1(name, length);

    e = root->createElement("head");
    e.setAttribute("left", "");
    //e.setAttribute("center", s);  // BUG?!? (Werner)
    e.setAttribute("center", "");   // replaced by this line :)
    e.setAttribute("right", "");
    paper.appendChild(e);
  }
  return true;
}

bool XMLTree::_hlink(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_horizontalpagebreaks(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_imdata(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_label(Q_UINT16 /*size*/, QDataStream& body)
{
  Q_UINT16 row, column, xf, length;

  body >> row >> column >> xf >> length;
  QDomElement e = root->createElement("cell");
  e.appendChild(getFormat(xf));

  char *name = new char[length];
  body.readRawBytes(name, length);
  QString s = QString::fromLatin1(name, length);
  e.setAttribute("row", (int) ++row);
  e.setAttribute("column", (int) ++column);
  QDomElement text = root->createElement("text");
  text.appendChild(root->createTextNode(s));
  e.appendChild(text);
  table->appendChild(e);

  delete []name;
  return true;
}

bool XMLTree::_labelsst(Q_UINT16 /*size*/, QDataStream& body)
{
  Q_UINT16 row, column, xf;
  Q_UINT32 isst;

  body >> row >> column >> xf >> isst;
  QDomElement e = root->createElement("cell");
  e.appendChild(getFormat(xf));

  e.setAttribute("row", (int) ++row);
  e.setAttribute("column", (int) ++column);
  QDomElement text = root->createElement("text");
  text.appendChild(root->createTextNode(*sst[isst]));
  e.appendChild(text);
  table->appendChild(e);
  return true;
}

bool XMLTree::_leftmargin(Q_UINT16 /*size*/, QDataStream& body)
{
  double value;
  body >> value;
  (paper.namedItem("borders")).toElement().setAttribute("left", value);

  return true;
}

bool XMLTree::_mulblank(Q_UINT16 size, QDataStream& body)
{
  Q_UINT16 row, xf, last, first;
  
  body >> row >> first;
  last = (size-6)/2;
  for (int i=0; i < last-first; ++i) {
    body >> xf;
    QDomElement e = root->createElement("cell");
    e.appendChild(getFormat(xf));
    e.setAttribute("row", row+1);
    e.setAttribute("column", first+i+1);
    table->appendChild(e);
  }
  return true;
}

bool XMLTree::_mulrk(Q_UINT16 size, QDataStream& body)
{
  double value = 0;

  QString s;

  Q_UINT16 first, last, row, xf;
  Q_UINT32 number, t[2];

  body >> row >> first;
  last = (size-6)/6;
  for (int i=0; i < last-first; ++i) {
    body >> xf >> number;

    switch (number & 0x03) {
    case 0:
      t[0] = 0;
      t[1] = number & 0xfffffffc;
      value = *((double*) t);
      break;
    case 1:
      t[0] = 0;
      t[1] = number & 0xfffffffc;
      value = *((double*) t) / 100;
      break;
    case 2:
      value = (double) (number >> 2);
      break;
    case 3:
      value = (double) (number >> 2) / 100;
      break;
    }

    switch (xfs[xf]->ifmt) {
    case 14: // Dates
    case 15:
    case 16:
    case 17:
    case 164: // These are undocumented
    case 168:
    case 165:
    case 166:
    case 167:
    case 169:
    case 170:
    case 171:
      int year, month, day;
      getDate((int) value, year, month, day);
      s.sprintf("%d/%d/%d", year, month, day);
      break;
    default: // Number
      s = QString::number(value, 'f');
      break;
    }

    QDomElement e = root->createElement("cell");
    e.appendChild(getFormat(xf));
    e.setAttribute("row", row+1);
    e.setAttribute("column", first+i+1);

    QDomElement text = root->createElement("text");
    text.appendChild(root->createTextNode(s));

    e.appendChild(text);
    table->appendChild(e);
  }

  return true;
}

bool XMLTree::_name(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_note(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_number(Q_UINT16 /*size*/, QDataStream& body)
{
  double value;
  QString s;
  Q_UINT16 row, column, xf;
  body >> row >> column >> xf >> value;

  QDomElement e = root->createElement("cell");
  e.appendChild(getFormat(xf));
  s = QString::number(value, 'f');
  e.setAttribute("row", (int) ++row);
  e.setAttribute("column", (int) ++column);
  QDomElement text = root->createElement("text");
  text.appendChild(root->createTextNode(s));
  e.appendChild(text);
  table->appendChild(e);

  return true;
}

bool XMLTree::_pane(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_paramqry(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_password(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_protect(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_qsi(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_recipname(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_rightmargin(Q_UINT16 /*size*/, QDataStream& body)
{
  double value;
  body >> value;
  (paper.namedItem("borders")).toElement().setAttribute("right", value);

  return true;
}

bool XMLTree::_rk(Q_UINT16 /*size*/, QDataStream& body)
{
  double value = 0;

  QString s;

  Q_UINT32 number, t[2];
  Q_UINT16 row, column, xf;

  body >> row >> column >> xf >> number;

  switch (number & 0x03) {
  case 0:
    t[0] = 0;
    t[1] = number & 0xfffffffc;
    value = *((double*) t);
    break;
  case 1:
    t[0] = 0;
    t[1] = number & 0xfffffffc;
    value = *((double*) t) / 100;
    break;
  case 2:
    value = (double) (number >> 2);
    break;
  case 3:
    value = (double) (number >> 2) / 100;
    break;
  }
  
  switch (xfs[xf]->ifmt) {
  case 14: // Dates
  case 15:
  case 16:
  case 17:
  case 164: // These are undocumented
  case 168:
  case 165:
  case 166:
  case 167:
  case 169:
  case 170:
  case 171:
    int year, month, day;
    getDate((int) value, year, month, day);
    s.sprintf("%d/%d/%d", year, month, day);
    break;
  default: // Number
    s = QString::number(value, 'f');
    break;
  }

  QDomElement e = root->createElement("cell");
  e.appendChild(getFormat(xf));
  e.setAttribute("row", (int) ++row);
  e.setAttribute("column", (int) ++column);

  QDomElement text = root->createElement("text");
  text.appendChild(root->createTextNode(s));

  e.appendChild(text);
  table->appendChild(e);

  return true;
}

bool XMLTree::_row(Q_UINT16 /*size*/, QDataStream& body)
{
  Q_UINT16 rowNr, skip, height;

  body >> rowNr >> skip >> skip >> height;

  QDomElement row = root->createElement("row");
  row.setAttribute("row", (int) rowNr + 1);
  row.setAttribute("height", (int) height / 40);
  table->appendChild(row);

  return true;
}

bool XMLTree::_scl(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_setup(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_shrfmla(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_sort(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_sst(Q_UINT16 /*size*/, QDataStream& body)
{
  char *name;
  QString *s;
  Q_UINT8 grbit;
  Q_UINT16 cch;
  Q_UINT32 count, skip;

  body >> count >> skip;
  for (int i=0; i < (int) count; ++i) {
    body >> cch >> grbit;
    name = new char[cch+1];
    body.readRawBytes(name, cch);
    name[cch]='\0';    
    s = new QString(name);
    sst.insert(i, s);
    delete[] name;
  }

  return true;
}

bool XMLTree::_standardwidth(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_string(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_tabid(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_tabidconf(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_table(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_topmargin(Q_UINT16 /*size*/, QDataStream& body)
{
  double value;
  body >> value;
  (paper.namedItem("borders")).toElement().setAttribute("top", value);

  return true;
}

bool XMLTree::_vcenter(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_verticalpagebreaks(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_window1(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_window2(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_writeaccess(Q_UINT16, QDataStream& body)
{
  Q_UINT8 length;

  body >> length;

  if (biff == BIFF_8) {
    Q_UINT16 skip;
    body >> skip;
  }

  char *name = new char[length];
  body.readRawBytes(name, length);
  QString s = QString::fromLatin1(name, length);
  doc.setAttribute("author", s);

  delete []name;

  return true;
}

bool XMLTree::_writeprot(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_wsbool(Q_UINT16, QDataStream&)
{
  return true;
}

bool XMLTree::_xf(Q_UINT16 /*size*/, QDataStream& body)
{
  xf_rec *x = new xf_rec;
  body >> x->ifnt >> x->ifmt >> x->attr >> x->align >> x->indent;
  body >> x->borderStyle >> x->sideBColor >> x->topBColor >> x->cellColor;
  xfs.insert(xfCount++, x);

  return true;
}

