// This file is part of the KDE project
// SPDX-FileCopyrightText: 1998-2002 The KSpread Team <calligra-devel@kde.org>
// SPDX-FileCopyrightText: 2005 Tomas Mecir <mecirt@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-only

// built-in logical functions
#include "LogicModule.h"

#include "engine/Function.h"
#include "engine/ValueCalc.h"
#include "engine/ValueConverter.h"

using namespace Calligra::Sheets;

// prototypes (sorted alphabetically)
Value func_and(valVector args, ValueCalc *calc, FuncExtra *);
Value func_false(valVector args, ValueCalc *calc, FuncExtra *);
Value func_if(valVector args, ValueCalc *calc, FuncExtra *);
Value func_iferror(valVector args, ValueCalc *calc, FuncExtra *);
Value func_ifna(valVector args, ValueCalc *calc, FuncExtra *);
Value func_nand(valVector args, ValueCalc *calc, FuncExtra *);
Value func_nor(valVector args, ValueCalc *calc, FuncExtra *);
Value func_not(valVector args, ValueCalc *calc, FuncExtra *);
Value func_or(valVector args, ValueCalc *calc, FuncExtra *);
Value func_true(valVector args, ValueCalc *calc, FuncExtra *);
Value func_xor(valVector args, ValueCalc *calc, FuncExtra *);

CALLIGRA_SHEETS_EXPORT_FUNCTION_MODULE("kspreadlogicmodule.json", LogicModule)

LogicModule::LogicModule(QObject *parent, const QVariantList &)
    : FunctionModule(parent)
{
    Function *f;

    f = new Function("FALSE", func_false);
    f->setParamCount(0);
    add(f);
    f = new Function("TRUE", func_true);
    f->setParamCount(0);
    add(f);
    f = new Function("NOT", func_not);
    f->setParamCount(1);
    add(f);
    f = new Function("AND", func_and);
    f->setParamCount(1, -1);
    f->setAcceptArray();
    add(f);
    f = new Function("NAND", func_nand);
    f->setParamCount(1, -1);
    f->setAcceptArray();
    add(f);
    f = new Function("NOR", func_nor);
    f->setParamCount(1, -1);
    f->setAcceptArray();
    add(f);
    f = new Function("OR", func_or);
    f->setParamCount(1, -1);
    f->setAcceptArray();
    add(f);
    f = new Function("XOR", func_xor);
    f->setParamCount(1, -1);
    f->setAcceptArray();
    add(f);
    f = new Function("IF", func_if);
    f->setParamCount(2, 3);
    add(f);
    f = new Function("IFERROR", func_iferror);
    f->setParamCount(2);
    add(f);
    f = new Function("IFNA", func_ifna);
    f->setParamCount(2);
    add(f);
}

QString LogicModule::descriptionFileName() const
{
    return QString("logic.xml");
}

// helper for most logical functions
static bool asBool(Value val, ValueCalc *calc, bool *ok = nullptr)
{
    return calc->conv()->asBoolean(val, ok).asBoolean();
}

///////////////////////////////////////////////////////////////////////////////

//
// ArrayWalker: AND
//
void awAnd(ValueCalc *calc, Value &res, Value value, Value)
{
    if (res.asBoolean())
        res = Value(asBool(value, calc));
}

//
// ArrayWalker: OR
//
void awOr(ValueCalc *calc, Value &res, Value value, Value)
{
    if (!res.asBoolean())
        res = Value(asBool(value, calc));
}

//
// ArrayWalker: XOR
//
void awXor(ValueCalc *calc, Value &count, Value value, Value)
{
    if (asBool(value, calc))
        count = Value(count.asInteger() + 1);
}

///////////////////////////////////////////////////////////////////////////////

//
// Function: AND
//
Value func_and(valVector args, ValueCalc *calc, FuncExtra *)
{
    Value result(true);
    int cnt = args.count();
    for (int i = 0; i < cnt; ++i) {
        if (args[i].isError())
            return args[i];
    }
    for (int i = 0; i < cnt; ++i) {
        calc->arrayWalk(args[i], result, awAnd, Value(0));
        if (!result.asBoolean())
            // if any value is false, return false
            return result;
    }
    // nothing is false -> return true
    return result;
}

//
// Function: FALSE
//
Value func_false(valVector, ValueCalc *, FuncExtra *)
{
    return Value(false);
}

//
// Function: IF
//
Value func_if(valVector args, ValueCalc *calc, FuncExtra *)
{
    if ((args[0].isError()))
        return args[0];
    bool ok = true;
    bool guard = asBool(args[0], calc, &ok);
    if (!ok)
        return Value::errorVALUE();
    if (guard)
        return args[1];
    // evaluated to false
    if (args.count() == 3) {
        if (args[2].isNull()) {
            return Value(0);
        } else {
            return args[2];
        }
    } else {
        // only two arguments
        return Value(false);
    }
}

//
// Function: IFERROR
//
Value func_iferror(valVector args, ValueCalc *, FuncExtra *)
{
    if (args[0].isError())
        return args[1];
    return args[0];
}

//
// Function: IFNA
//
Value func_ifna(valVector args, ValueCalc *, FuncExtra *)
{
    if (args[0] == Value::errorNA())
        return args[1];
    return args[0];
}

//
// Function: NAND
//
Value func_nand(valVector args, ValueCalc *calc, FuncExtra *extra)
{
    // AND in reverse
    return Value(!func_and(args, calc, extra).asBoolean());
}

//
// Function: NOR
//
Value func_nor(valVector args, ValueCalc *calc, FuncExtra *extra)
{
    // OR in reverse
    return Value(!func_or(args, calc, extra).asBoolean());
}

//
// Function: NOT
//
Value func_not(valVector args, ValueCalc *calc, FuncExtra *)
{
    if (args[0].isError())
        return args[0];

    bool ok = true;
    bool val = !asBool(args[0], calc, &ok);
    if (!ok)
        return Value::errorVALUE();
    return Value(val);
}

//
// Function: OR
//
Value func_or(valVector args, ValueCalc *calc, FuncExtra *)
{
    Value result(false);
    int cnt = args.count();
    for (int i = 0; i < cnt; ++i) {
        if (args[i].isError())
            return args[i];
    }
    for (int i = 0; i < cnt; ++i) {
        calc->arrayWalk(args[i], result, awOr, Value(0));
        if (result.asBoolean())
            // if any value is true, return true
            return result;
    }
    // nothing is true -> return false
    return result;
}

//
// Function: TRUE
//
Value func_true(valVector, ValueCalc *, FuncExtra *)
{
    return Value(true);
}

//
// Function: XOR
//
Value func_xor(valVector args, ValueCalc *calc, FuncExtra *)
{
    // exclusive OR - odd number of values must be true
    int cnt = args.count();
    Value count(0);
    for (int i = 0; i < cnt; ++i) {
        if (args[i].isError())
            return args[i];
    }
    for (int i = 0; i < cnt; ++i)
        calc->arrayWalk(args[i], count, awXor, Value(0));
    return Value((count.asInteger() & 1) == 1);
}

#include "logic.moc"
