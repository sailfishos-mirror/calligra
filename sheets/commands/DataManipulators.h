// This file is part of the KDE project
// SPDX-FileCopyrightText: 2006 Tomas Mecir <mecirt@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-only


#ifndef CALLIGRA_SHEETS_DATA_MANIPULATORS
#define CALLIGRA_SHEETS_DATA_MANIPULATORS

#include "AbstractRegionCommand.h"
#include "Style.h"
#include "Value.h"

#include "sheets_common_export.h"


namespace Calligra
{
namespace Sheets
{

/**
 * \ingroup Commands
 * \brief Abstract command for setting values.
 */
class CALLIGRA_SHEETS_COMMON_EXPORT AbstractDataManipulator : public AbstractRegionCommand
{
public:
    explicit AbstractDataManipulator(KUndo2Command *parent = 0);
    ~AbstractDataManipulator() override;

    bool process(Element* element) override;

protected:
    /** Return new value. row/col are relative to sheet, not element.
    If the function sets *parse to true, the value will be treated as an
    user-entered string and parsed by Cell. */
    virtual Value newValue(Element *element, int col, int row,
                           bool *parse, Format::Type *fmtType) = 0;

    /** do we want to change this cell ? */
    virtual bool wantChange(Element *element, int col, int row) {
        Q_UNUSED(element)
        Q_UNUSED(col)
        Q_UNUSED(row)
        return true;
    }

    /**
     * Starts the undo recording.
     */
    bool preProcessing() override;

    /**
     * Processes the region. Calls process(Element*).
     */
    bool mainProcessing() override;

    /**
     * Stops the undo recording and stores the old data.
     */
    bool postProcessing() override;
};

/**
 * \ingroup Commands
 * \brief Abstract command for setting values/styles.
 */
class AbstractDFManipulator : public AbstractDataManipulator
{
public:
    explicit AbstractDFManipulator(KUndo2Command *parent = 0);
    ~AbstractDFManipulator() override;
    bool process(Element* element) override;

    /** returns whether this manipulator changes formats */
    bool changeFormat() {
        return m_changeformat;
    }
    /** set whether this manipulator changes formats */
    void setChangeFormat(bool chf) {
        m_changeformat = chf;
    }
protected:
    /** this method should return new format for a given cell */
    virtual Style newFormat(Element *element, int col, int row) = 0;

    bool m_changeformat : 1;
};


/**
 * \ingroup Commands
 * \brief Sets values of a cell range.
 */
class DataManipulator : public AbstractDataManipulator
{
public:
    explicit DataManipulator(KUndo2Command *parent = 0);
    ~DataManipulator() override;
    void setParsing(bool val) {
        m_parsing = val;
    }
    void setExpandMatrix(bool expand) {
        m_expandMatrix = expand;
    }
    /** set the values for the range. Can be either a single value, or
    a value array */
    void setValue(Value val) {
        m_data = val;
    }
    /** If set, all cells shall be switched to this format. If parsing is
    true, the resulting value may end up being different. */
    void setFormat(Format::Type fmtType) {
        m_format = fmtType;
    }
protected:
    bool preProcessing() override;
    bool process(Element* element) override;
    Value newValue(Element *element, int col, int row, bool *, Format::Type *) override;
    bool wantChange(Element *element, int col, int row) override;

    Value m_data;
    Format::Type m_format;
    bool m_parsing : 1;
    bool m_expandMatrix : 1;
};


/**
 * \ingroup Commands
 * \brief Fills a value series into a cell range.
 */
class SeriesManipulator : public AbstractDataManipulator
{
public:
    enum Series { Column, Row, Linear, Geometric };

    SeriesManipulator();
    ~SeriesManipulator() override;

    /** Setup the series. This sets the necessary parameters, and also the
    correct range. */
    void setupSeries(const QPoint &_marker, double start, double end,
                     double step, Series mode, Series type);
protected:
    Value newValue(Element *element, int col, int row, bool *,
                           Format::Type *) override;

    Series m_type;
    Value m_start, m_step, m_prev;
    int m_last;
};


/**
 * \ingroup Commands
 * \brief Fills values into a cell range.
 */
class FillManipulator : public AbstractDFManipulator
{
public:
    FillManipulator();
    ~FillManipulator() override;

    enum Direction { Up = 0, Down, Left, Right };

    void setDirection(Direction d) {
        m_dir = d;
    }
protected:
    Value newValue(Element *element, int col, int row,
                           bool *parse, Format::Type *fmtType) override;
    Style newFormat(Element *element, int col, int row) override;
    Direction m_dir;
};


/**
 * \ingroup Commands
 * \brief Converts string values to upper-/lowercase.
 */
class CaseManipulator: public AbstractDataManipulator
{
public:
    CaseManipulator();
    ~CaseManipulator() override;

    enum CaseMode {
        Upper = 0,
        Lower,
        FirstUpper
    };
    void changeMode(CaseMode mode) {
        m_mode = mode;
    }
    void changeLowerCase();
    void changeFirstUpper();
protected:
    Value newValue(Element *element, int col, int row,
                           bool *parse, Format::Type *fmtType) override;

    /** do we want to change this cell ? */
    bool wantChange(Element *element, int col, int row) override;

    CaseMode m_mode;
};


/**
 * \ingroup Commands
 * \brief Inserts/Removes cells by shifting other cells.
 */
class ShiftManipulator : public AbstractRegionCommand
{
public:
    enum Direction { ShiftRight, ShiftBottom };
    explicit ShiftManipulator(KUndo2Command *parent = 0);
    ~ShiftManipulator() override;
    void setDirection(Direction direction) {
        m_direction = direction;
    }
    void setReverse(bool reverse) override;

protected:
    bool process(Element*) override;
    bool preProcessing() override;
    bool mainProcessing() override;
    bool postProcessing() override;

private:
    Direction m_direction;

    enum Mode { Insert, Delete };
    Mode m_mode;
};

} // namespace Sheets
} // namespace Calligra

#endif  // CALLIGRA_SHEETS_DATA_MANIPULATORS
