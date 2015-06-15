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
 *     std::cout << d.toString << std::endl;
 */
class Decimal128 {
public:
    /**
    * This struct holds the raw data for IEEE 754-2008 data types
    */
    struct Decimal128Value {
        uint64_t high64;
        uint64_t low64;
    };

    enum RoundingModes {
        kRoundTiesToEven = 0,
        kRoundTowardNegative = 1,
        kRoundTowardPositive = 2,
        kRoundTowardZero = 3,
        kRoundTiesToAway = 4
    };

    Decimal128();
    /**
      * This constructor takes in a raw decimal128 type, which consists of two
      * uint64_t's. This class performs an endian check on the system to ensure
      * that the Decimal128Value.high64 represents the higher 64 bits.
      */
    Decimal128(Decimal128Value value);
    Decimal128(int32_t i);
    Decimal128(int64_t l);
    /**
      * This constructor takes a double and constructs a Decimal128 object
      * given a roundMode with a fixed precision of 15. Doubles can only
      * properly represent a decimal precision of 15-17 digits.
      * The general idea is to quantize the direct double->dec128 conversion
      * with a quantum of 1E(-15 +/- base10 exponent equivalent of the double).
      * To do this, we find the smallest (abs value) base 10 exponent greater
      * than the double's base 2 exp and shift the quantizer's exp accordingly.
      */
    Decimal128(double d, RoundingModes roundMode = kRoundTiesToEven);
    /**
      * This constructor takes a string and constructs a Decimal128 object from it.
      * Inputs larger than 34 digits of precision are rounded according to the
      * specified rounding mode. The following (and variations) are all accepted:
      * +2.02E200
      * 2.02E+200
      * -202E-500
      * somethingE200 --> NaN
      * 200E9999999999 --> +Inf
      * -200E9999999999 --> -Inf
      */
    Decimal128(std::string s, RoundingModes roundMode = kRoundTiesToEven);
    ~Decimal128();

    /**
     * These functions get and set the two 64 bit arrays storing the
     * decimal128 value, which is useful for direct manipulation and testing.
     */
    const Decimal128Value getValue() const;

    /**
      * This set of functions converts a decimal128 to a certain type with a
      * given rounding mode.
      */
    int32_t toInt(RoundingModes roundMode = kRoundTiesToEven);
    int64_t toLong(RoundingModes roundMode = kRoundTiesToEven);
    double toDouble(RoundingModes roundMode = kRoundTiesToEven);
    std::string toString();
    /**
      * This set of functions converts a Decial128 to a certain type and
      * returns a <value, boolean> pair where the boolean represents whether
      * the conversion has been performed exactly. In other words, it returns
      * whether the Decimal128 is truly an int, long, or double.
      */
    std::pair<int32_t, bool> isAndToInt(RoundingModes roundMode = kRoundTiesToEven);
    std::pair<int64_t, bool> isAndToLong(RoundingModes roundMode = kRoundTiesToEven);
    std::pair<double, bool> isAndToDouble(RoundingModes roundMode = kRoundTiesToEven);

    // Mathematical operations
    Decimal128 add(const Decimal128& dec128, RoundingModes roundMode = kRoundTiesToEven);
    Decimal128 subtract(const Decimal128& dec128, RoundingModes roundMode = kRoundTiesToEven);
    Decimal128 multiply(const Decimal128& dec128, RoundingModes roundMode = kRoundTiesToEven);
    Decimal128 divide(const Decimal128& dec128, RoundingModes roundMode = kRoundTiesToEven);
    // This function quantizes the current decimal given a quantum reference
    Decimal128 quantize(const Decimal128& reference, RoundingModes roundMode = kRoundTiesToEven);

    // Comparison operations
    bool compareEqual(const Decimal128& dec128);
    bool compareNotEqual(const Decimal128& dec128);
    bool compareGreater(const Decimal128& dec128);
    bool compareGreaterEqual(const Decimal128& dec128);
    bool compareLess(const Decimal128& dec128);
    bool compareLessEqual(const Decimal128& dec128);

private:
    Decimal128Value _value;
};
}