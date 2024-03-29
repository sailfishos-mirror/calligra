// This file is part of KSpread
// SPDX-FileCopyrightText: 2007 Sebastian Sauer <mail@dipe.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef SCRIPTINGWRITER_H
#define SCRIPTINGWRITER_H

// These need to be here as the MOC references this file.
//
#include "ScriptingModule.h"

#include <engine/Region.h>
#include <engine/CS_Time.h>
#include <core/Cell.h>
#include <core/Sheet.h>
#include <core/Map.h>
#include <part/Doc.h>

/**
* The ScriptingWriter class provides abstract high-level functionality to write
* content to KSpread sheets and to manipulate the content of cells.
*
* The following python sample demonstrates how to use the ScriptingWriter to
* write content to KSpread.
* \code
* # Import the KSpread module
* import KSpread
* # Create a writer instance.
* writer = KSpread.writer()
* # Set the sheet we like to write to.
* sheetname = "Sheet2"
* if not writer.setSheet(sheetname):
*     raise "Invalid sheet \"%s\" defined." % sheetname
* # Set the cell(s) we like to start to write to. If this
* # is not defined, writing starts from A1.
* cellname = "B12"
* if not writer.setCell(cellname):
*     raise "Invalid cell \"%s\" defined." % cellname
* # Now fill the cells with some content.
* for record in [ ["One,"Two"] , ["Three,"Four"] ]:
*     # Write the tuple to the current row.
*     if not writer.setValues(record):
*         raise "Failed to set record %s" % record
*     # Go to the next row.
*     writer.next()
* \endcode
*/
class ScriptingWriter : public QObject
{
    Q_OBJECT
public:

    /**
    * Constructor.
    * \param module The \a ScriptingModule instance that should be used
    * as datasource for this writer. The writer will operate on the
    * document those module provides us.
    */
    explicit ScriptingWriter(ScriptingModule* module) : QObject(module), m_module(module), m_cell(0) {
        clearAll();
    }

    /**
    * Destructor.
    */
    ~ScriptingWriter() override {}

public Q_SLOTS:

    /**
    * \return the current sheetname the writer is on. All operations done with
    * the writer are done on this sheet.
    */
    QString sheet() {
        return m_sheet ? m_sheet->sheetName() : QString();
    }

    /**
    * Set the current sheetname the writer is on to \p sheetname . If there exist
    * no sheet with such a sheetname false is returned else, so on success, true
    * is returned.
    */
    bool setSheet(const QString& sheetname) {
        Calligra::Sheets::Sheet* s = dynamic_cast<Calligra::Sheets::Sheet *>(m_module->kspreadDoc()->map()->findSheet(sheetname));
        if (! s) return false;
        clearAll();
        m_sheet = s;
        return true;
    }

    /**
    * \return the current cellname the writer is on. Operations like for example
    * the value() and setValue() methods are done on the defined sheet in the
    * defined cell. You may like to use it to manipulate the content of an
    * explicit cell.
    */
    QString cell() {
        return Calligra::Sheets::Cell::name(m_column, m_row);
    }

    /**
    * Set the current cellname the writer is on to \p cellname . If such a cell
    * exist true is returned else, e.g. if the cellname was just wrong, false got
    * returned.
    */
    bool setCell(const QString& cellname) {
        if (! m_sheet) return false;
        const Calligra::Sheets::Region region = m_sheet->doc()->map()->regionFromName(cellname, m_sheet);
        if (region.firstRange().isNull()) return false;
        QPoint point = region.firstRange().topLeft();
        m_column = point.x();
        m_row = point.y();
        clearCell();
        return true;
    }

    /**
    * Return the current row number.
    */
    int row() {
        return m_row;
    }

    /**
    * Set the current row number to \p rownumber .
    */
    void setRow(int rownumber) {
        m_row = rownumber;
        clearCell();
    }

    /**
    * Return the current column number.
    */
    int column() {
        return m_column;
    }

    /**
    * Set the current column number to \p columnnumber .
    */
    void setColumn(int columnnumber) {
        m_column = columnnumber;
        clearCell();
    }

    //int cellCount() {}
    //int rowCount() {}
    //int columnCount() {}

    //enum Filter { Any, NotEmpty, Numeric, Date, Time, String, Formula, Comment };
    //QString filter() {}
    //void setFilter(const QString& filter) {}

    //enum Direction { Cell, Row, Column };
    //QString direction() {}
    //void setDirection(const QString& direction) {}

    //void first() {}
    //void last() {}
    //void prev() {}

    /**
    * Go to the next row.
    */
    void next() {
        m_row++;
        clearCell();
    }

    //QVariant value() {}
    //QVariantList values() {}

    /**
    * Set the value of the current cell.
    *
    * \param value The value that should be set.
    * \param parse If this is true, the default, then the
    * value got parsed to look for the type else we
    * assume the value has the correct type.
    * \return true if the value was set successful else
    * false is returned.
    */
    bool setValue(const QVariant& value, bool parse = true) {
        Calligra::Sheets::Value v;
        if (parse)
            v = Calligra::Sheets::Value(value.toString());
        else {
            const Calligra::Sheets::CalculationSettings* settings = m_module->kspreadDoc()->map()->calculationSettings();
            switch (value.type()) {
            case QVariant::Invalid:     v = Calligra::Sheets::Value(); break;
            case QVariant::Bool:        v = Calligra::Sheets::Value(value.toBool()); break;
            case QVariant::Int:         v = Calligra::Sheets::Value(value.toInt()); break;
            case QVariant::ULongLong:   v = Calligra::Sheets::Value((int64_t)value.toLongLong()); break;
            case QVariant::Double:      v = Calligra::Sheets::Value(value.toDouble()); break;
            case QVariant::String:      v = Calligra::Sheets::Value(value.toString()); break;
            case QVariant::Date:        v = Calligra::Sheets::Value(value.toDate(), settings); break;
            case QVariant::Time:        v = Calligra::Sheets::Value(Calligra::Sheets::Time(value.toTime())); break;
            case QVariant::DateTime:    v = Calligra::Sheets::Value(value.toDateTime(), settings); break;
            default: return false;
            }
        }
        //Calligra::Sheets::Cell* c = getCell();
        //c->setValue(v);
        Calligra::Sheets::Cell cell(m_sheet, m_column, m_row);
        if (!parse) {
            cell.setCellValue(v);
        } else {
            cell.parseUserInput(value.toString());
        }
        return true;
    }

    /**
    * Set the values of the cells in the current row.
    *
    * \param values The list of values that should be set.
    * \param parse If this is true, the default, then the
    * value got parsed to look for the type else we
    * assume the value has the correct type.
    * \return true if the values got set successful else
    * false is returned.
    */
    bool setValues(const QVariantList& values, bool parse = true) {
        bool ok = true;
        const int prevcolumn = m_column;
        m_module->doc()->beginMacro(kundo2_i18n("Set Values"));
        foreach(QVariant v, values) {
            if (! setValue(v, parse)) ok = false;
            m_column++;
            clearCell();
        }
        m_module->doc()->endMacro();
        m_column = prevcolumn;
        return ok;
    }
    //bool insertValue(const QVariant& value) {}
    //bool insertValues(const QVariantList& value) {}

private:
    Q_DISABLE_COPY(ScriptingWriter)

    ScriptingModule* const m_module;
    Calligra::Sheets::Sheet* m_sheet;
    int m_column, m_row;
    Calligra::Sheets::Cell* m_cell;

    Calligra::Sheets::Cell* getCell() {
        Q_ASSERT(m_sheet);
        if (m_cell) return m_cell;
        m_cell = new Calligra::Sheets::Cell(m_sheet, m_column, m_row);
        return m_cell;
    }

    void clearCell() {
        delete m_cell;
        m_cell = 0;
    }

    void clearAll() {
        m_sheet = 0;
        m_column = 0;
        m_row = 0;
        clearCell();
    }
};

#endif
