/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2004 Nicolas GOUTTE <goutte@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "csvimport.h"

#include <QByteArray>
#include <QFile>
#include <QRegExp>
#include <QVector>
#include <QGuiApplication>
#include <QDebug>

#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <klocale.h>

#include <KoCsvImportDialog.h>
#include <KoFilterChain.h>
#include <KoFilterManager.h>

#include <sheets/ElapsedTime_p.h>
#include <sheets/CalculationSettings.h>
#include <sheets/Cell.h>
#include <sheets/part/Doc.h>
#include <sheets/Global.h>
#include <sheets/Map.h>
#include <sheets/RowColumnFormat.h>
#include <sheets/Sheet.h>
#include <sheets/Style.h>
#include <sheets/Value.h>
#include <sheets/ValueConverter.h>

using namespace Calligra::Sheets;

// hehe >:->

/*
 To generate a test CSV file:

 perl -e '$i=0;while($i<30000) { print rand().",".rand()."\n"; $i++ }' > file.csv
*/

K_PLUGIN_FACTORY_WITH_JSON(CSVImportFactory, "calligra_filter_csv2sheets.json", registerPlugin<CSVFilter>();)

Q_LOGGING_CATEGORY(lcCsvImport, "calligra.filter.csv.import")

CSVFilter::CSVFilter(QObject* parent, const QVariantList&) :
        KoFilter(parent)
{
}

KoFilter::ConversionStatus CSVFilter::convert(const QByteArray& from, const QByteArray& to)
{
    QString file(m_chain->inputFile());
    KoDocument* document = m_chain->outputDocument();

    if (!document)
        return KoFilter::StupidError;

    if (!qobject_cast<const Calligra::Sheets::Doc *>(document)) {
        qWarning(lcCsvImport) << "document isn't a Calligra::Sheets::Doc but a " << document->metaObject()->className();
        return KoFilter::NotImplemented;
    }
    if ((from != "text/csv" && from != "text/plain") || to != "application/vnd.oasis.opendocument.spreadsheet") {
        qWarning(lcCsvImport) << "Invalid mimetypes " << from << " " << to;
        return KoFilter::NotImplemented;
    }

    Doc *ksdoc = static_cast<Doc *>(document);   // type checked above

//    if (ksdoc->mimeType() != "application/vnd.oasis.opendocument.spreadsheet") {
//       qWarning(lcCsvImport) << "Invalid document mimetype " << ksdoc->mimeType();
//        return KoFilter::NotImplemented;
//    }

    QFile in(file);
    if (!in.open(QIODevice::ReadOnly)) {
        KMessageBox::sorry(0L, i18n("CSV filter cannot open input file - please report."));
        in.close();
        return KoFilter::FileNotFound;
    }

    // ###### FIXME: disabled for now
    //QString csv_delimiter;
    //if (!config.isNull())
    //    csv_delimiter = config[0];

    QByteArray inputFile(in.readAll());
    in.close();

    KoCsvImportDialog* dialog = new KoCsvImportDialog(0);
    dialog->setData(inputFile);
    dialog->setDecimalSymbol(ksdoc->map()->calculationSettings()->locale()->decimalSymbol());
    dialog->setThousandsSeparator(ksdoc->map()->calculationSettings()->locale()->thousandsSeparator());
    if (!m_chain->manager()->getBatchMode() && !dialog->exec())
        return KoFilter::UserCancelled;
    inputFile.resize(0);   // Release memory (input file content)

    ElapsedTime t("Filling data into document");

    Sheet *sheet = ksdoc->map()->addNewSheet();

    int numRows = dialog->rows();
    int numCols = dialog->cols();

    if (numRows == 0)
        ++numRows;

    // Initialize the decimal symbol and thousands separator to use for parsing.
    const QString documentDecimalSymbol = ksdoc->map()->calculationSettings()->locale()->decimalSymbol();
    const QString documentThousandsSeparator = ksdoc->map()->calculationSettings()->locale()->thousandsSeparator();
    ksdoc->map()->calculationSettings()->locale()->setDecimalSymbol(dialog->decimalSymbol());
    ksdoc->map()->calculationSettings()->locale()->setThousandsSeparator(dialog->thousandsSeparator());

    int step = 100 / numRows * numCols;
    int value = 0;

    emit sigProgress(value);
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    const double defaultWidth = ksdoc->map()->defaultColumnFormat()->width();
    QVector<double> widths(numCols);
    for (int i = 0; i < numCols; ++i)
        widths[i] = defaultWidth;

    Cell cell(sheet, 1, 1);
    QFontMetrics fm(cell.style().font());

    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            value += step;
            emit sigProgress(value);
            const QString text(dialog->text(row, col));

            // ### FIXME: how to calculate the width of numbers (as they might not be in the right format)
            const double len = fm.width(text);
            if (len > widths[col])
                widths[col] = len;

            cell = Cell(sheet, col + 1, row + 1);

            switch (dialog->dataType(col)) {
            case KoCsvImportDialog::Generic:
            default: {
                cell.parseUserInput(text);
                break;
            }
            case KoCsvImportDialog::Text: {
                Value value(text);
                cell.setValue(value);
                cell.setUserInput(ksdoc->map()->converter()->asString(value).asString());
                break;
            }
            case KoCsvImportDialog::Date: {
                Value value(text);
                cell.setValue(ksdoc->map()->converter()->asDate(value));
                cell.setUserInput(ksdoc->map()->converter()->asString(value).asString());
                break;
            }
            case KoCsvImportDialog::Currency: {
                Value value(text);
                value.setFormat(Value::fmt_Money);
                cell.setValue(value);
                cell.setUserInput(ksdoc->map()->converter()->asString(value).asString());
                break;
            }
            case KoCsvImportDialog::None: {
                // just skip the content
                break;
            }
            }
        }
    }

    emit sigProgress(98);

    for (int i = 0; i < numCols; ++i) {
        if (widths[i] > defaultWidth)
            sheet->nonDefaultColumnFormat(i + 1)->setWidth(widths[i]);
    }

    // Restore the document's decimal symbol and thousands separator.
    ksdoc->map()->calculationSettings()->locale()->setDecimalSymbol(documentDecimalSymbol);
    ksdoc->map()->calculationSettings()->locale()->setThousandsSeparator(documentThousandsSeparator);

    emit sigProgress(100);
    QGuiApplication::restoreOverrideCursor();
    delete dialog;

    return KoFilter::OK;
}

#include <csvimport.moc>
