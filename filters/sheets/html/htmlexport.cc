/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Eva Brucherseifer <eva@kde.org>
   SPDX-FileCopyrightText: 2005 Bram Schoenmakers <bramschoenmakers@kde.nl>
   based on kspread csv export filter by David Faure

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "htmlexport.h"
#include "exportdialog.h"
#include "calligra-version.h"

#include <KPluginFactory>
#include <KoFilterChain.h>
#include <KoFilterManager.h>
#include <KoDocumentInfo.h>

#include <sheets/engine/CellBaseStorage.h>
#include <sheets/engine/Util.h>
#include <sheets/core/Cell.h>
#include <sheets/core/DocBase.h>
#include <sheets/core/Map.h>
#include <sheets/core/Sheet.h>
#include <sheets/core/Style.h>

using namespace Calligra::Sheets;

Q_LOGGING_CATEGORY(lcHtml, "calligra.filter.html")

K_PLUGIN_FACTORY_WITH_JSON(HTMLExportFactory, "calligra_filter_sheets2html.json",
                           registerPlugin<HTMLExport>();)

const QString html_table_tag = "table";
const QString html_table_options = QString(" border=\"%1\" cellspacing=\"%2\"");
const QString html_row_tag = "tr";
const QString html_row_options = "";
const QString html_cell_tag = "td";
const QString html_cell_options = "";
const QString html_bold = "b";
const QString html_italic = "i";
const QString html_underline = "u";
const QString html_right = "right";
const QString html_left = "left";
const QString html_center = "center";
const QString html_top = "top";
const QString html_bottom = "bottom";
const QString html_middle = "middle";
const QString html_h1 = "h1";

HTMLExport::HTMLExport(QObject* parent, const QVariantList&) :
        KoFilter(parent), m_dialog(new ExportDialog())
{
}

HTMLExport::~HTMLExport()
{
    delete m_dialog;
}

// HTML entities, AFAIK we don't need to escape " to &quot; (dnaber):
const QString strAmp("&amp;");
const QString nbsp("&nbsp;");
const QString strLt("&lt;");
const QString strGt("&gt;");

// The reason why we use the KoDocument* approach and not the QDomDocument
// approach is because we don't want to export formulas but values !
KoFilter::ConversionStatus HTMLExport::convert(const QByteArray& from, const QByteArray& to)
{
    if (to != "text/html" || from != "application/x-kspread") {
        qWarning(lcHtml) << "Invalid mimetypes " << to << " " << from;
        return KoFilter::NotImplemented;
    }

    KoDocument* document = m_chain->inputDocument();

    if (!document)
        return KoFilter::StupidError;

    if (!::qobject_cast<const Calligra::Sheets::DocBase *>(document)) {   // it's safer that way :)
        qWarning(lcHtml) << "document isn't a Calligra::Sheets::DocBase but a " << document->metaObject()->className();
        return KoFilter::NotImplemented;
    }

    const DocBase * ksdoc = dynamic_cast<const DocBase *>(document);

    if (ksdoc->mimeType() != "application/x-kspread") {
        qWarning(lcHtml) << "Invalid document mimetype " << ksdoc->mimeType();
        return KoFilter::NotImplemented;
    }

    QString filenameBase = m_chain->outputFile();
    filenameBase = filenameBase.left(filenameBase.lastIndexOf('.'));

    QStringList sheets;
    for(SheetBase* bsheet : ksdoc->map()->sheetList()) {
        Sheet *sheet = dynamic_cast<Sheet *>(bsheet);
        int rows = 0;
        int columns = 0;
        detectFilledCells(sheet, rows, columns);
        m_rowmap[ sheet->sheetName()] = rows;
        m_columnmap[ sheet->sheetName()] = columns;

        if (rows > 0 && columns > 0) {
            sheets.append(sheet->sheetName());
        }
    }
    m_dialog->setSheets(sheets);
    if (!m_chain->manager()->getBatchMode() ) {
        if (m_dialog->exec() == QDialog::Rejected) {
            return KoFilter::UserCancelled;
        }
    }

    sheets = m_dialog->sheets();
    QString str;
    for (int i = 0; i < sheets.count() ; ++i) {
        SheetBase *bsheet = ksdoc->map()->findSheet(sheets[i]);
        Sheet *sheet = dynamic_cast<Sheet *>(bsheet);
        if (!sheet)
            continue;

        QString file = fileName(filenameBase, sheet->sheetName(), sheets.count() > 1);

        if (m_dialog->separateFiles() || sheets[i] == sheets.first()) {
            str.clear();
            openPage(sheet, document, str);
            writeTOC(sheets, filenameBase, str);
        }

        convertSheet(sheet, str, m_rowmap[ sheet->sheetName()], m_columnmap[ sheet->sheetName()]);

        if (m_dialog->separateFiles() || sheets[i] == sheets.last()) {
            closePage(str);
            QFile out(file);
            if (!out.open(QIODevice::WriteOnly)) {
                qWarning(lcHtml) << "Unable to open output file!" << Qt::endl;
                out.close();
                return KoFilter::FileNotFound;
            }
            QTextStream streamOut(&out);
            streamOut << str << Qt::endl;
            out.close();
        }

        if (!m_dialog->separateFiles()) {
            createSheetSeparator(str);
        }

    }

    Q_EMIT sigProgress(100);
    return KoFilter::OK;
}

void HTMLExport::openPage(Sheet *sheet, KoDocument *document, QString &str)
{
    QString title;
    KoDocumentInfo *info = document->documentInfo();
    if (info && !info->aboutInfo("title").isEmpty())
        title = info->aboutInfo("title") + " - ";
    title += sheet->sheetName();

    // header
    str = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
          " \"http://www.w3.org/TR/html4/loose.dtd\"> \n"
          "<html>\n"
          "<head>\n"
          "<meta http-equiv=\"Content-Type\" " +
          QString("content=\"text/html; charset=\"UTF-8\">\n") +
          "<meta name=\"Generator\" "
          "content=\"KSpread HTML Export Filter Version = " + QLatin1StringView(CALLIGRA_VERSION_STRING) + "\">\n";

    // Insert stylesheet
    if (!m_dialog->customStyleURL().isEmpty()) {
        str += "<link ref=\"stylesheet\" type=\"text/css\" href=\"" +
               m_dialog->customStyleURL().url() +
               "\" title=\"Style\" >\n";
    }

    str += "<title>" + title + "</title>\n"
           "</head>\n" +
           QString("<body bgcolor=\"#FFFFFF\" dir=\"%1\">\n").arg(
               (sheet->layoutDirection() == Qt::RightToLeft) ? "rtl" : "ltr") +

           "<a name=\"__top\">\n";
}

void HTMLExport::closePage(QString &str)
{
    str += "<p align=\"" + html_center + "\"><a href=\"#__top\">" + i18n("Top") + "</a></p>\n"
           "</body>\n"
           "</html>\n\n";
}

void HTMLExport::convertSheet(Sheet *sheet, QString &str, int iMaxUsedRow, int iMaxUsedColumn)
{
    QString emptyLines;

    // Either we get hold of KSpreadTable::m_dctCells and apply the old method below (for sorting)
    // or, cleaner and already sorted, we use KSpreadTable's API (slower probably, though)
    int iMaxRow = sheet->cellStorage()->rows();

    if (!m_dialog->separateFiles())
        str += "<a name=\"" + sheet->sheetName().toLower().trimmed() + "\">\n";

    str += ("<h1>" + sheet->sheetName() + "</h1><br>\n");

    // this is just a bad approximation which fails for documents with less than 50 rows, but
    // we don't need any progress stuff there anyway :) (Werner)
    int value = 0;
    int step = iMaxRow > 50 ? iMaxRow / 50 : 1;
    int i = 1;

    str += '<' + html_table_tag + html_table_options.arg(m_dialog->useBorders() ? "1" : "0").arg(m_dialog->pixelsBetweenCells()) +
           QString("dir=\"%1\">\n").arg((sheet->layoutDirection() == Qt::RightToLeft) ? "rtl" : "ltr");

    unsigned int nonempty_cells_prev = 0;

    for (int currentrow = 1 ; currentrow <= iMaxUsedRow ; ++currentrow, ++i) {
        if (i > step) {
            value += 2;
            Q_EMIT sigProgress(value);
            i = 0;
        }

        QString line;
        unsigned int nonempty_cells = 0;

        for (int currentcolumn = 1 ; currentcolumn <= iMaxUsedColumn ; currentcolumn++) {
            Cell cell(sheet, currentcolumn, currentrow);
            const Style style = cell.effectiveStyle();
            if (cell.needsPrinting())
                nonempty_cells++;
            QString text;
            // FIXME: some formatting seems to be missing with cell.userInput(), e.g.
            // "208.00" in KSpread will be "208" in HTML (not always?!)
            bool link = false;

            if (!cell.link().isEmpty()) {
                if (Util::localReferenceAnchor(cell.link())) {
                    text = cell.userInput();
                } else {
                    text = " <A href=\"" + cell.link() + "\">" + cell.userInput() + "</A>";
                    link = true;
                }
            } else
                text = cell.displayText();
#if 0
            switch (cell.content()) {
            case Cell::Text:
                text = cell.userInput();
                break;
            case Cell::RichText:
            case Cell::VisualFormula:
                text = cell.userInput(); // untested
                break;
            case Cell::Formula:
                cell.calc(true);   // Incredible, cells are not calculated if the document was just opened
                text = cell.valueString();
                break;
            }
            text = cell.prefix(currentrow, currentcolumn) + ' ' + text + ' '
                   + cell.postfix(currentrow, currentcolumn);
#endif
            line += "  <" + html_cell_tag + html_cell_options;
            if (text.isRightToLeft() != (sheet->layoutDirection() == Qt::RightToLeft))
                line += QString(" dir=\"%1\" ").arg(text.isRightToLeft() ? "rtl" : "ltr");
            const QColor bgcolor = style.backgroundColor();
            if (bgcolor.isValid() && bgcolor.name() != "#ffffff") // change color only for non-white cells
                line += " bgcolor=\"" + bgcolor.name() + "\"";

            switch ((Style::HAlign)cell.effectiveAlignX()) {
            case Style::Left:
                line += " align=\"" + html_left + "\"";
                break;
            case Style::Right:
                line += " align=\"" + html_right + "\"";
                break;
            case Style::Center:
                line += " align=\"" + html_center + "\"";
                break;
            case Style::HAlignUndefined:
            case Style::Justified:
                break;
            }
            switch ((Style::VAlign) style.valign()) {
            case Style::Top:
                line += " valign=\"" + html_top + "\"";
                break;
            case Style::Middle:
                line += " valign=\"" + html_middle + "\"";
                break;
            case Style::Bottom:
                line += " valign=\"" + html_bottom + "\"";
                break;
            case Style::VAlignUndefined:
            case Style::VJustified:
            case Style::VDistributed:
                break;
            }
            line += " width=\"" + QString::number(cell.width()) + "\"";
            line += " height=\"" + QString::number(cell.height()) + "\"";

            if (cell.mergedXCells() > 0) {
                QString tmp;
                int extra_cells = cell.mergedXCells();
                line += " colspan=\"" + tmp.setNum(extra_cells + 1) + "\"";
                currentcolumn += extra_cells;
            }
            text = text.trimmed();
            if (!text.isEmpty() && text.at(0) == '!') {
                // this is supposed to be markup, just remove the '!':
                text = text.right(text.length() - 1);
            } else if (!link) {
                // Escape HTML characters.
                text.replace('&' , strAmp)
                .replace('<' , strLt)
                .replace('>' , strGt)
                .replace(' ' , nbsp);
            }
            line += ">\n";

            if (style.bold()) {
                text.insert(0, '<' + html_bold + '>');
                text.append("</" + html_bold + '>');
            }
            if (style.italic()) {
                text.insert(0, '<' + html_italic + '>');
                text.append("</" + html_italic + '>');
            }
            if (style.underline()) {
                text.insert(0, '<' + html_underline + '>');
                text.append("</" + html_underline + '>');
            }
            QColor textColor = style.fontColor();
            if (textColor.isValid() && textColor.name() != "#000000") { // change color only for non-default text
                text.insert(0, "<font color=\"" + textColor.name() + "\">");
                text.append("</font>");
            }
            line += ' ' + text +
                    "\n  </" + html_cell_tag + ">\n";
        }

        if (nonempty_cells == 0 && nonempty_cells_prev == 0) {
            nonempty_cells_prev = nonempty_cells;
            // skip line if there's more than one empty line
            continue;
        } else {
            nonempty_cells_prev = nonempty_cells;
            str += emptyLines +
                   '<' + html_row_tag + html_row_options + ">\n" +
                   line +
                   "</" + html_row_tag + '>';
            emptyLines.clear();
            // Append a CR, but in a temp string -> if no other real line,
            // then those will be dropped
            emptyLines += '\n';
        }
    }
    str += "\n</" + html_table_tag + ">\n<br>\n";
}

void HTMLExport::createSheetSeparator(QString &str)
{
    str += "<p align=\"" + html_center + "\"><a href=\"#__top\">" + i18n("Top") + "</a></p>\n"
           "<hr width=\"80%\">\n";
}

void HTMLExport::writeTOC(const QStringList &sheets, const QString &base, QString &str)
{
    // don't create TOC for 1 sheet
    if (sheets.count() == 1)
        return;

    str += "<p align=\"" + html_center + "\">\n";

    for (int i = 0 ; i < sheets.count() ; ++i) {
        str += "<a href=\"";

        if (m_dialog->separateFiles()) {
            str += fileName(base, sheets[i], sheets.count() > 1);
        } else {
            str += '#' + sheets[i].toLower().trimmed();
        }

        str += "\">" + sheets[i] + "</a>\n";
        if (i != sheets.count() - 1)
            str += " - ";
    }

    str += "</p><hr width=\"80%\">\n";
}

QString HTMLExport::fileName(const QString &base, const QString &sheetName, bool multipleFiles)
{
    QString fileName = base;
    if (m_dialog->separateFiles() && multipleFiles) {
        fileName += '-' + sheetName;
    }
    fileName += ".html";

    return fileName;
}

void HTMLExport::detectFilledCells(Sheet *sheet, int &rows, int &columns)
{
    int iMaxColumn = sheet->cellStorage()->columns();
    int iMaxRow = sheet->cellStorage()->rows();

    rows = 0;
    columns = 0;

    for (int currentrow = 1 ; currentrow <= iMaxRow ; ++currentrow) {
        Cell cell;
        int iUsedColumn = 0;
        for (int currentcolumn = 1 ; currentcolumn <= iMaxColumn ; currentcolumn++) {
            cell = Cell(sheet, currentcolumn, currentrow);
            if (!cell.isDefault() && !cell.isEmpty()) {
                iUsedColumn = currentcolumn;
            }
        }
        if (!cell.isNull())
            iUsedColumn += cell.mergedXCells();
        if (iUsedColumn > columns)
            columns = iUsedColumn;
        if (iUsedColumn > 0)
            rows = currentrow;
    }
}

#include <htmlexport.moc>
