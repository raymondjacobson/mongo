/*    Copyright 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <array>
#include <string>
#include <third_party/IntelRDFPMathLib20U1/LIBRARY/src/bid_conf.h>
#include <third_party/IntelRDFPMathLib20U1/LIBRARY/src/bid_functions.h>
#include <utility>

namespace mongo {

/**
 * Wrapper class for the MongoDB Decimal128 data type. Sample usage:
 *     Decimal128 d("+10.0");
 *     std::cout << d.toString() << std::endl;
 */
class Decimal128 {
public:
    /**
     * This struct holds the raw data for IEEE 754-2008 data types
     */
    struct Decimal128Value {
        uint64_t high64;
        uint64_t low64;

        /**
         * Constructors for Decimal128Value
         * Default to zero, copy constructor, and from size 2 uint64_t array
         */
        Decimal128Value();
        Decimal128Value(const Decimal128Value& dval);
        Decimal128Value(const uint64_t dval[2]);
    };

    enum RoundingMode {
        kRoundTiesToEven = 0,
        kRoundTowardNegative = 1,
        kRoundTowardPositive = 2,
        kRoundTowardZero = 3,
        kRoundTiesToAway = 4
    };

    /**
     * Default initialize Decimal128's value struct to zero
     */
    Decimal128();

    /**
     * This constructor takes in a raw decimal128 type, which consists of two
     * uint64_t's. This class performs an endian check on the system to ensure
     * that the Decimal128Value.high64 represents the higher 64 bits.
     */
    Decimal128(Decimal128Value dec128Value);
    Decimal128(int32_t int32Value);
    Decimal128(int64_t int64Value);

    /**
     * This constructor takes a double and constructs a Decimal128 object
     * given a roundMode with a fixed precision of 15. Doubles can only
     * properly represent a decimal precision of 15-17 digits.
     * The general idea is to quantize the direct double->dec128 conversion
     * with a quantum of 1E(-15 +/- base10 exponent equivalent of the double).
     * To do this, we find the smallest (abs value) base 10 exponent greater
     * than the double's base 2 exp and shift the quantizer's exp accordingly.
     */
    Decimal128(double doubleValue, RoundingMode roundMode = kRoundTiesToEven);

    /**
     * This constructor takes a string and constructs a Decimal128 object from it.
     * Inputs larger than 34 digits of precision are rounded according to the
     * specified rounding mode. The following (and variations) are all accepted:
     * "+2.02E200"
     * "2.02E+200"
     * "-202E-500"
     * "somethingE200" --> NaN
     * "200E9999999999" --> +Inf
     * "-200E9999999999" --> -Inf
     */
    Decimal128(std::string stringValue, RoundingMode roundMode = kRoundTiesToEven);
    ~Decimal128();

    /**
     * These functions get the inner Decimal128Value struct storing the
     * decimal128 value, which is useful for direct manipulation and testing.
     * Const cast away for the mutable version of the function.
     */
    const Decimal128Value& getValue() const;
    Decimal128Value& getValue();

    /**
     * This set of functions converts a Decimal128 to a certain numeric type with a
     * given rounding mode.
     */
    int32_t toInt(RoundingMode roundMode = kRoundTiesToEven);
    int64_t toLong(RoundingMode roundMode = kRoundTiesToEven);
    double toDouble(RoundingMode roundMode = kRoundTiesToEven);

    /**
     * This function converts a Decimal128 to a string with syntax similar to the
     * Decimal128 string constructor.
     */
    std::string toString();

    /**
     * This set of functions converts a Decimal128 to a certain numerical type and
     * returns a <value, boolean> pair where the boolean represents whether
     * the conversion has been performed exactly. In other words, it returns
     * whether the Decimal128 is truly an int, long, or double.
     */
    std::pair<int32_t, bool> isAndToInt(RoundingMode roundMode = kRoundTiesToEven);
    std::pair<int64_t, bool> isAndToLong(RoundingMode roundMode = kRoundTiesToEven);
    std::pair<double, bool> isAndToDouble(RoundingMode roundMode = kRoundTiesToEven);

    /**
     * This set of functions check whether a Decimal128 is NaN or +/- Inf
     */
    bool isNaN();
    bool isInfinite();
    bool isNegative();

    /**
     * This set of mathematical operation functions takes a single Decimal128
     * and a rounding mode parameter and performs the operation to the calling Decimal128.
     * The calls are commutative in that a.add(b) is equivalent to b.add(a).
     * When operation results trigger a precision greater than 34 decimal digits,
     * the supplied rounding mode is invoked (defeaulting to kRoundTiesToEven).
     * NaN and +/- Inf are valid arguments and return values.
     */
    Decimal128 add(const Decimal128& rhs, RoundingMode roundMode = kRoundTiesToEven);
    Decimal128 subtract(const Decimal128& rhs, RoundingMode roundMode = kRoundTiesToEven);
    Decimal128 multiply(const Decimal128& rhs, RoundingMode roundMode = kRoundTiesToEven);
    Decimal128 divide(const Decimal128& rhs, RoundingMode roundMode = kRoundTiesToEven);

    /**
     * This function quantizes the current decimal given a quantum reference
     */
    Decimal128 quantize(const Decimal128& reference, RoundingMode roundMode = kRoundTiesToEven);

    /**
     * This set of comparison operations takes a single Decimal128 and returns a boolean
     * noting the value of the comparison. These comparisons are not total ordered, but
     * comply with the IEEE 754-2008 spec. The comparison returns true if the caller
     * is <equal, notequal, greater, greaterequal, less, lessequal> the argument (rhs).
     */
    bool isEqual(const Decimal128& rhs);
    bool isNotEqual(const Decimal128& rhs);
    bool isGreater(const Decimal128& rhs);
    bool isGreaterEqual(const Decimal128& rhs);
    bool isLess(const Decimal128& rhs);
    bool isLessEqual(const Decimal128& rhs);

private:
    Decimal128Value _value;
};

}  // namespace mongo