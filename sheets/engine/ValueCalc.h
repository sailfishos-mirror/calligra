/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Tomas Mecir <mecirt@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_VALUECALC
#define CALLIGRA_SHEETS_VALUECALC

#include "Value.h"

#include "sheets_engine_export.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace Calligra
{
namespace Sheets
{
class CellBase;
class ValueCalc;
class ValueConverter;

// Condition structures
enum Comp {
    isEqual,
    isLess,
    isGreater,
    lessEqual,
    greaterEqual,
    notEqual,
    stringMatch,
    regexMatch,
    wildcardMatch
};
enum Type {
    numeric,
    string
};

struct Condition {
    Comp comp;
    int index;
    Number value;
    QString stringValue;
    Type type;
};

typedef void (*arrayWalkFunc)(ValueCalc *, Value &result, Value val, Value param);
// A function that can map an array element-wise
typedef Value (ValueCalc::*arrayMapFunc)(const Value &val, const Value &param);

/**
 * \ingroup Value
The ValueCalc class is used to perform all sorts of calculations.

Usage of this class for simpler calculations is deprecated, as we now use
the Number object directly for that. This class is to be used for computations
of more complicated and ranged functions.
*/

class CALLIGRA_SHEETS_ENGINE_EXPORT ValueCalc
{
public:
    explicit ValueCalc(ValueConverter *c);

    ValueConverter *conv()
    {
        return converter;
    }

    const CalculationSettings *settings() const;

    /** basic arithmetic operations */
    Value add(const Value &a, const Value &b);
    Value sub(const Value &a, const Value &b);
    Value mul(const Value &a, const Value &b);
    Value div(const Value &a, const Value &b);
    Value mod(const Value &a, const Value &b);
    Value pow(const Value &a, const Value &b);
    Value sqr(const Value &a);
    Value sqrt(const Value &a);
    Value add(const Value &a, Number b);
    Value sub(const Value &a, Number b);
    Value mul(const Value &a, Number b);
    Value div(const Value &a, Number b);
    Value pow(const Value &a, Number b);
    Value abs(const Value &a);

    /** comparison and related */
    bool isZero(const Value &a);
    bool isEven(const Value &a);
    /** numerical comparison */
    bool equal(const Value &a, const Value &b);
    /** numerical comparison with a little epsilon tolerance */
    bool approxEqual(const Value &a, const Value &b);
    /** numerical comparison */
    bool greater(const Value &a, const Value &b);
    /** numerical comparison - greater or equal */
    bool gequal(const Value &a, const Value &b);
    /** numerical comparison */
    bool lower(const Value &a, const Value &b);
    /** string comparison */
    bool strEqual(const Value &a, const Value &b, bool CalcS = true);
    /** string comparison */
    bool strGreater(const Value &a, const Value &b, bool CalcS = true);
    /** string comparison - greater or equal */
    bool strGequal(const Value &a, const Value &b, bool CalcS = true);
    /** string comparison */
    bool strLower(const Value &a, const Value &b, bool CalcS = true);
    /** string or numerical comparison */
    bool naturalEqual(const Value &a, const Value &b, bool CalcS = true);
    /** string or numerical comparison */
    bool naturalGreater(const Value &a, const Value &b, bool CalcS = true);
    /** string or numerical comparison - greater or equal */
    bool naturalGequal(const Value &a, const Value &b, bool CalcS = true);
    /** string or numerical comparison */
    bool naturalLower(const Value &a, const Value &b, bool CalcS = true);
    /** string or numerical comparison - lower or equal */
    bool naturalLequal(const Value &a, const Value &b, bool CalcS = true);

    int sign(const Value &a);

    // just a quick workaround
    Value add(Number a, const Value &b)
    {
        return add(Value(a), b);
    }
    Value sub(Number a, const Value &b)
    {
        return sub(Value(a), b);
    }
    Value mul(Number a, const Value &b)
    {
        return mul(Value(a), b);
    }
    Value div(Number a, const Value &b)
    {
        return div(Value(a), b);
    }
    Value pow(Number a, const Value &b)
    {
        return pow(Value(a), b);
    }

    bool equal(const Value &a, Number b)
    {
        return equal(a, Value(b));
    }
    bool greater(const Value &a, Number b)
    {
        return greater(a, Value(b));
    }
    bool lower(const Value &a, Number b)
    {
        return lower(a, Value(b));
    }
    bool equal(Number a, const Value &b)
    {
        return equal(Value(a), b);
    }
    bool greater(Number a, const Value &b)
    {
        return greater(Value(a), b);
    }
    bool lower(Number a, const Value &b)
    {
        return lower(Value(a), b);
    }

    /** rounding */
    Value roundDown(const Value &a, const Value &digits);
    Value roundUp(const Value &a, const Value &digits);
    Value round(const Value &a, const Value &digits);
    Value roundDown(const Value &a, int digits = 0);
    Value roundUp(const Value &a, int digits = 0);
    Value round(const Value &a, int digits = 0);

    /** logarithms and exponentials */
    Value log(const Value &number, const Value &base);
    Value log(const Value &number, Number base = 10);
    Value ln(const Value &number);
    Value exp(const Value &number);

    /** constants */
    Value pi();
    Value eps();

    /** random number from <0.0, range) */
    Value random(Number range = 1.0);
    Value random(Value range);

    /** some computational functions */
    Value fact(const Value &which);
    Value fact(const Value &which, const Value &end);
    Value fact(int which, int end = 0);
    /** Number factorial (every other number multiplied) */
    Value factDouble(int which);
    Value factDouble(Value which);

    /** combinations */
    Value combin(int n, int k);
    Value combin(Value n, Value k);

    /** greatest common divisor */
    Value gcd(const Value &a, const Value &b);
    /** lowest common multiplicator */
    Value lcm(const Value &a, const Value &b);

    /** base conversion 10 -> base */
    Value base(const Value &val, int base = 16, int prec = 0, int minLength = 0);
    /** base conversion base -> 10 */
    Value fromBase(const Value &val, int base = 16);

    /** goniometric functions */
    Value sin(const Value &number);
    Value cos(const Value &number);
    Value tg(const Value &number);
    Value cotg(const Value &number);
    Value asin(const Value &number);
    Value acos(const Value &number);
    Value atg(const Value &number);
    Value atan2(const Value &y, const Value &x);

    /** hyperbolic functions */
    Value sinh(const Value &number);
    Value cosh(const Value &number);
    Value tgh(const Value &number);
    Value asinh(const Value &number);
    Value acosh(const Value &number);
    Value atgh(const Value &number);

    /** some statistical stuff
      TODO: we may want to move these over to a separate class or something,
      as the functions are mostly big */
    Value phi(Value x);
    Value gauss(Value xx);
    Value gaussinv(Value xx);
    Value GetGamma(Value _x);
    Value GetLogGamma(Value _x);
    Value GetGammaDist(Value _x, Value _alpha, Value _beta);
    Value GetBeta(Value _x, Value _alpha, Value _beta);

    /** bessel functions - may also end up being separated from here */
    Value besseli(Value v, Value x);
    Value besselj(Value v, Value x);
    Value besselk(Value v, Value x);
    Value besseln(Value v, Value x);

    /** error functions (see: man erf) */
    Value erf(Value x);
    Value erfc(Value x);

    /** array/range walking */
    void arrayWalk(const Value &range, Value &res, arrayWalkFunc func, Value param);
    /** Walk the array in function-like style.
    This method is here to avoid duplication in function handlers. */
    void arrayWalk(QVector<Value> &range, Value &res, arrayWalkFunc func, Value param);
    Value arrayMap(const Value &array, arrayMapFunc func, const Value &param);
    Value twoArrayMap(const Value &array1, arrayMapFunc func, const Value &array2);
    void twoArrayWalk(const Value &a1, const Value &a2, Value &res, arrayWalkFunc func);
    void twoArrayWalk(QVector<Value> &a1, QVector<Value> &a2, Value &res, arrayWalkFunc func);
    arrayWalkFunc awFunc(const QString &name);
    void registerAwFunc(const QString &name, arrayWalkFunc func);

    /** basic range functions */
    // if full is true, A-version is used (means string/bool values included)
    Value sum(const Value &range, bool full = true);
    Value sumsq(const Value &range, bool full = true);
    Value sumIf(const Value &range, const Condition &cond);
    Value sumIf(const CellBase &sumRangeStart, const Value &checkRange, const Condition &cond);
    Value sumIfs(const CellBase &sumRangeStart, QList<Value> c_Range, QList<Condition> cond, const float limit);
    Value averageIf(const Value &range, const Condition &cond);
    Value averageIf(const CellBase &avgRangeStart, const Value &checkRange, const Condition &cond);
    Value averageIfs(const CellBase &avgRangeStart, QList<Value> c_Range, QList<Condition> cond, const float limit);
    int count(const Value &range, bool full = true);
    int countIf(const Value &range, const Condition &cond);
    Value countIfs(const CellBase &cntRangeStart, QList<Value> c_range, QList<Condition> cond, const float limit);
    Value avg(const Value &range, bool full = true);
    Value max(const Value &range, bool full = true);
    Value min(const Value &range, bool full = true);
    Value product(const Value &range, Value init, bool full = true);
    Value stddev(const Value &range, bool full = true);
    Value stddev(const Value &range, Value avg, bool full = true);
    Value stddevP(const Value &range, bool full = true);
    Value stddevP(const Value &range, Value avg, bool full = true);

    /** range functions using value lists */
    Value sum(QVector<Value> range, bool full = true);
    int count(QVector<Value> range, bool full = true);
    Value avg(QVector<Value> range, bool full = true);
    Value max(QVector<Value> range, bool full = true);
    Value min(QVector<Value> range, bool full = true);
    Value product(QVector<Value> range, Value init, bool full = true);
    Value stddev(QVector<Value> range, bool full = true);
    Value stddev(QVector<Value> range, Value avg, bool full = true);
    Value stddevP(QVector<Value> range, bool full = true);
    Value stddevP(QVector<Value> range, Value avg, bool full = true);

    int days360(int day1, int month1, int year1, bool leapYear1, int day2, int month2, int year2, bool leapYear2, bool usaMethod);
    int days360(const QDate &_date1, const QDate &_date2, bool european);
    Value yearFrac(const QDate &startDate, const QDate &endDate, int basis);

    /**
      This method parses the condition in string text to the condition cond.
      It sets the condition's type and value.
    */
    void getCond(Condition &cond, Value val);

    /**
      Returns true if value d matches the condition cond, built with getCond().
      Otherwise, it returns false.
    */
    bool matches(const Condition &cond, Value d);

    /** return formatting for the result, based on formattings of input values */
    Value::Format format(Value a, Value b);

protected:
    ValueConverter *converter;

    /** registered array-walk functions */
    std::map<QString, arrayWalkFunc> awFuncs;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_VALUECALC
