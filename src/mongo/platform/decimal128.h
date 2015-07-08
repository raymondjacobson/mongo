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

#include "mongo/platform/cstdint.h"

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
        uint64_t low64;
        uint64_t high64;

        /**
         * Constructors for Decimal128Value
         * Default to zero, and from size 2 uint64_t array
         */
        Decimal128Value();
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
     * The signaling flags enum is to be used to compare against when an optional
     * signaling flags argument is passed by reference to a decimal operation function.
     * The values of these flags are defined in the intel RDFP math library.
     *
     * Decimal128 dcml = Decimal128('0.1');
     * uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
     * double dbl = dcml.toDouble(&sigFlag);
     * if (sigFlag == SignalingFlag::kInexact)
     *     cout << "inexact decimal to double conversion!" << endl;
     */
    enum SignalingFlag {
        kNoFlag = 0x00,
        kInexact = 0x20,
        kUnderflow = 0x10,
        kOverflow = 0x08,
        kDivideByZero = 0x04,
        kInvalid = 0x01
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

    /**
     * These functions get the inner Decimal128Value struct storing the
     * decimal128 value, which is useful for direct manipulation and testing.
     * Const cast away for the mutable version of the function.
     */
    const Decimal128Value& getValue() const;
    Decimal128Value& getValue();

    /**
     * This set of functions converts a Decimal128 to a certain integer type with a
     * given rounding mode.
     *
     * Each function is overloaded to provide an optional signalingFlag argument
     * that can be set to one of the Decimal128::SignalingFlag enumerators:
     * kNoFlag, kInvalid
     *
     * Note: The signaling flags for these functions only signal
     * an invalid conversion. If inexact conversion flags are necessary, call
     * the toTypeExact version of the function defined below. This set of operations
     * has better performance than the latter.
     */
    int32_t toInt(RoundingMode roundMode = kRoundTiesToEven) const;
    int32_t toInt(uint32_t* signalingFlag, RoundingMode roundMode = kRoundTiesToEven) const;
    int64_t toLong(RoundingMode roundMode = kRoundTiesToEven) const;
    int64_t toLong(uint32_t* signalingFlag, RoundingMode roundMode = kRoundTiesToEven) const;

    /**
     * This set of functions converts a Decimal128 to a certain integer type with a
     * given rounding mode. The signaling flags for these functions will also signal
     * inexact computation.
     *
     * Each function is overloaded to provide an optional signalingFlag argument
     * that can be set to one of the Decimal128::SignalingFlag enumerators:
     * kNoFlag, kInexact, kInvalid
     */
    int32_t toIntExact(RoundingMode roundMode = kRoundTiesToEven) const;
    int32_t toIntExact(uint32_t* signalingFlag, RoundingMode roundMode = kRoundTiesToEven) const;
    int64_t toLongExact(RoundingMode roundMode = kRoundTiesToEven) const;
    int64_t toLongExact(uint32_t* signalingFlag, RoundingMode roundMode = kRoundTiesToEven) const;

    /**
     * These functions convert decimals to doubles and have the ability to signal
     * inexact, underflow, overflow, and invalid operation.
     *
     * This function is overloaded to provide an optional signalingFlag argument
     * that can be set to one of the Decimal128::SignalingFlag enumerators:
     * kNoFlag, kInexact, kUnderflow, kOverflow, kInvalid
     */
    double toDouble(RoundingMode roundMode = kRoundTiesToEven) const;
    double toDouble(uint32_t* signalingFlag, RoundingMode roundMode = kRoundTiesToEven) const;

    /**
     * This function converts a Decimal128 to a string with syntax similar to the
     * Decimal128 string constructor.
     */
    std::string toString() const;

    /**
     * This set of functions check whether a Decimal128 is Zero, NaN, or +/- Inf
     */
    bool isZero() const;
    bool isNaN() const;
    bool isInfinite() const;
    bool isNegative() const;

    /**
     * This set of mathematical operation functions implement the corresponding
     * IEEE 754-2008 operations on self and other.
     * The operations are commutative, so a.add(b) is equivalent to b.add(a).
     * Rounding of results that require a precision greater than 34 decimal digits
     * is performed using the supplied rounding mode (defaulting to kRoundTiesToEven).
     * NaNs and infinities are handled according to the IEEE 754-2008 specification.
     *
     * Each function is overloaded to provide an optional signalingFlag argument
     * that can be set to one of the Decimal128::SignalingFlag enumerators:
     * kNoFlag, kInexact, kUnderflow, kOverflow, kInvalid
     *
     * The divide operation may also set signalingFlag to kDivideByZero
     */
    Decimal128 add(const Decimal128& other, RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 add(const Decimal128& other,
                   uint32_t* signalingFlag,
                   RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 subtract(const Decimal128& other, RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 subtract(const Decimal128& other,
                        uint32_t* signalingFlag,
                        RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 multiply(const Decimal128& other, RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 multiply(const Decimal128& other,
                        uint32_t* signalingFlag,
                        RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 divide(const Decimal128& other, RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 divide(const Decimal128& other,
                      uint32_t* signalingFlag,
                      RoundingMode roundMode = kRoundTiesToEven) const;

    /**
     * This function quantizes the current decimal given a quantum reference
     */
    Decimal128 quantize(const Decimal128& reference,
                        RoundingMode roundMode = kRoundTiesToEven) const;
    Decimal128 quantize(const Decimal128& reference,
                        uint32_t* signalingFlag,
                        RoundingMode roundMode = kRoundTiesToEven) const;

    /**
     * This set of comparison operations takes a single Decimal128 and returns a boolean
     * noting the value of the comparison. These comparisons are not total ordered, but
     * comply with the IEEE 754-2008 spec. The comparison returns true if the caller
     * is <equal, notequal, greater, greaterequal, less, lessequal> the argument (other).
     *
     * Note: Currently the supported library does not support signaling comparisons.
     * When it does overloaded signaling functions similar to the ones defined above
     * should be implemented.
     */
    bool isEqual(const Decimal128& other) const;
    bool isNotEqual(const Decimal128& other) const;
    bool isGreater(const Decimal128& other) const;
    bool isGreaterEqual(const Decimal128& other) const;
    bool isLess(const Decimal128& other) const;
    bool isLessEqual(const Decimal128& other) const;

    /**
     * These functions get the minimum and maximum valid Decimal128s
     * getPosMin() -> 1E-6176
     * getPosMax() -> 9999999999999999999999999999999999E6111
     * getNegMin() -> -9999999999999999999999999999999999E6111
     * getNegMax() -> -1E-6176
     */
    static Decimal128 getPosMin();
    static Decimal128 getPosMax();
    static Decimal128 getNegMin();
    static Decimal128 getNegMax();
    /**
     * These functions get special values (+/- Inf, +/- NaN) represented in Decimal128, which
     * is very useful for testing and other numerical comparisons
     */
    static Decimal128 getPosInfinity();
    static Decimal128 getNegInfinity();
    static Decimal128 getPosNaN();
    static Decimal128 getNegNaN();

private:
    Decimal128Value _value;
};

}  // namespace mongo
