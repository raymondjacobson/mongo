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

#include "mongo/platform/basic.h"

#include "mongo/platform/decimal128.h"

#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "mongo/platform/endian.h"
#include "mongo/util/assert_util.h"

namespace mongo {
// Determine system's endian ordering in order to construct decimal
// 128 values directly (inexpensively)
#if MONGO_CONFIG_BYTE_ORDER == 1234
int HIGH_64 = 1;
int LOW_64 = 0;
#else
int HIGH_64 = 0;
int LOW_64 = 1;
#endif

/**
 * This helper function creates a library specific type for the
 * IntelRDFPMathLib20U1 library from Decimal128's _value
 */
BID_UINT128 decimal128ToLibraryType(Decimal128::Decimal128Value value) {
    BID_UINT128 dec128;
    dec128.w[LOW_64] = value.low64;
    dec128.w[HIGH_64] = value.high64;
    return dec128;
}

/**
 * This helper function takes an intel decimal 128 library type and quantizes
 * it to 15 decimal digits.
 * BID_UINT128 value : the value to quantize
 * int base10Exp : the base 10 exponent of value to scale the quantizer by
 * RoundingMode roundMode : the rounding mode to be used for quantizing operations
 * uint32_t idec_signaling_flags : flags for signaling imprecise results
 */
BID_UINT128 quantizeTo15DecimalDigits(BID_UINT128 value,
                                      Decimal128::RoundingMode roundMode,
                                      int base10Exp,
                                      uint32_t& idec_signaling_flags) {
    BID_UINT128 quantizerReference;

    // The quantizer starts at 1E-15
    quantizerReference.w[HIGH_64] = 0x3022000000000000;
    quantizerReference.w[LOW_64] = 0x0000000000000001;

    // Scale the quantizer by the base 10 exponent. This is necessary to keep
    // the scale of the quantizer reference correct. For example, the decimal value 101
    // needs a different quantizer (1E-12) than the decimal value 1001 (1E-11) to yield
    // a 15 digit decimal precision.
    quantizerReference =
        bid128_scalbn(quantizerReference, base10Exp, roundMode, &idec_signaling_flags);

    value = bid128_quantize(value, quantizerReference, roundMode, &idec_signaling_flags);
    return value;
}

Decimal128::Decimal128Value::Decimal128Value() : high64(0), low64(0) {
}

Decimal128::Decimal128Value::Decimal128Value(const Decimal128Value& dval)
    : high64(dval.high64), low64(dval.low64) {
}

Decimal128::Decimal128Value::Decimal128Value(const uint64_t dval[2])
    : high64(dval[HIGH_64]), low64(dval[LOW_64]) {
}

Decimal128::Decimal128() : _value() {
}

Decimal128::Decimal128(Decimal128::Decimal128Value dec128Value) : _value(dec128Value) {
}

Decimal128::Decimal128(int32_t int32Value)
    : _value(Decimal128Value(bid128_from_int32(int32Value).w)) {
}

Decimal128::Decimal128(int64_t int64Value)
    : _value(Decimal128Value(bid128_from_int64(int64Value).w)) {
}

Decimal128::Decimal128(double doubleValue, RoundingMode roundMode) {
    BID_UINT128 convertedDoubleValue;
    uint32_t idec_signaling_flags = 0;
    convertedDoubleValue = binary64_to_bid128(doubleValue, roundMode, &idec_signaling_flags);

    // If the original number was zero, there's no need to quantize
    if (doubleValue == 0.0) {
        _value = Decimal128Value(convertedDoubleValue.w);
        return;
    }

    // Quantize the new number fixing its precision to exactly 15
    int exp;
    // Get the exponent from the incoming double
    frexp(doubleValue, &exp);
    /**
     * Convert a base 2 exponent to base 10 using integer arithmetic.
     *
     * Note: The following explanation is explained for positive N. For negative N,
     * similar logic holds.
     *
     * Given a double D with exponent E, we would like to find N such that 10^N >= |D|
     * and 10^(N-1) < |D|. We will use N = E * 301 / 1000 + 1 as a starting guess.
     *
     * This formula is derived from the fact that 10^(E*log10(2)) = 2^E.
     * We add one because in the majority of cases E * 301 / 1000 is an
     * underestimate since 301/1000 < log10(2), the integer division truncates, and, typically,
     * the bits of the mantissa of the considered double D are not filled with zeros after the
     * most significant bit.
     *
     * Take as an example: 2^7 = 128.
     * Following the forumla, N = 7 * 301 / 1000 + 1 = 3
     * 10^3 = 1000 > 2^7 > 10^2, therefore our guess of N = 3 was correct.
     *
     * If there exists an M = N-1 such that 10^M is also greater than D, our guess was
     * off and we will need to decrement N and re-quantize our value. This can occasionally happen
     * due to the greedy addition of 1 in the initial guess of N.
     * Fortunately, there is never a case where there exists an M = N-2 such that 10^M > D.
     *
     * This conclusion is reached based on knowledge that calculation
     * using the above formula is never inaccurate by an absolute error of more than 1.
     *
     * Total absolute error is caused by:
     *
     * - Rounding inaccuracy from using the fraction 0.301 instead of log10(2) = 0.301029...
     *   Max Absolute Error = Max(N) * RelError
     *                      = 308 * ((0.301 - log10(2)) / log10(2)) = -0.03069
     *
     * - Inaccuracy from the fact that our formula looks at comparing to 2^E instead of numbers
     *   up to but not including 2^(E+1)
     *   Max Absolute Error = -log10(2) = -0.30103
     *
     * - Integer arithmetic inaccuracy from one division (301/1000)
     *   Up until the integer division truncation, our total error is between -0.33072 and 0,
     *   which means after truncation our total error can be no more than -1. It is either 0 or -1.
     *
     * In the worst case, the total error is -1. In the case of such error, we must
     * subtract off 1 from our guess to account for the error and retry the quantizing operation.
     */

    // Hold off adding 1 because we treat +/- slightly differently
    int base10Exp = (exp * 301) / 1000;

    /**
     * Increase base10Exp by an additional 1 to get positive and negative
     * exponents to behave the same way with regard to precision.
     *
     * For example, if we had a double 1E+5 (100000) and we would like to produce a
     * decimal with exactly 15 digits of precision and the same value, we would want to quantize
     * by 10^-9 to get 100000000000000E-9 as a result.
     *
     * In the negative case, if we had double 1E-5 (.00001) and we would like to produce
     * a decimal with exactly 15 digits of precision, we would want to quantize
     * by 10^19 to get 100000000000000E-19.
     *
     * Our initial quantizer reference starts at 10^-15. In the positive case, we want
     * to scale the quantizer by 5 + 1 (10^-15 * 10^6 = 10^-9). In the negative case,
     * we want to scale the quantizer by -5 + 1 (10^-15 * 10^-4 = 10^-19).
     *
     * Since we still have to increase |base10Exp| by 1 for the above formula, in total
     * we will increment all positive base10Exp by 2. Negative base10Exp will be decremented
     * by 1 for the above formula and then incremented by 1 given the explanation in this
     * section, which leaves the value unchanged.
     */
    if (base10Exp > 0)
        base10Exp += 2;

    _value =
        Decimal128Value(quantizeTo15DecimalDigits(
                            convertedDoubleValue, roundMode, base10Exp, idec_signaling_flags).w);

    // Check if the quantization was done correctly: _value stores exactly 15
    // decimal digits of precision (15 digits can fit into the low 64 bits of the decimal)
    if (_value.low64 < 100000000000000ull || _value.low64 > 999999999999999ull) {
        // If we didn't precisely get 15 digits of precision, the original base 10 exponent
        // guess was 1 off (see comment above), so quantize once more with magnitude - 1
        if (base10Exp > 0)
            base10Exp--;
        else
            base10Exp++;
        _value = Decimal128Value(
            quantizeTo15DecimalDigits(
                convertedDoubleValue, roundMode, base10Exp, idec_signaling_flags).w);
    }
    invariant(_value.low64 >= 100000000000000ull && _value.low64 <= 999999999999999ull);
}

Decimal128::Decimal128(std::string stringValue, RoundingMode roundMode) {
    std::unique_ptr<char[]> charInput(new char[stringValue.size() + 1]);
    std::copy(stringValue.begin(), stringValue.end(), charInput.get());
    charInput[stringValue.size()] = '\0';
    BID_UINT128 dec128;
    uint32_t idec_signaling_flags = 0;
    dec128 = bid128_from_string(charInput.get(), roundMode, &idec_signaling_flags);
    _value = Decimal128Value(dec128.w);
}

Decimal128::~Decimal128() {
}

const Decimal128::Decimal128Value Decimal128::getValue() const {
    return _value;
}

int32_t Decimal128::toInt(RoundingMode roundMode) {
    BID_UINT128 dec128 = decimal128ToLibraryType(_value);
    uint32_t idec_signaling_flags = 0;
    switch (roundMode) {
        case kRoundTiesToEven:
            return bid128_to_int32_rnint(dec128, &idec_signaling_flags);
        case kRoundTowardNegative:
            return bid128_to_int32_floor(dec128, &idec_signaling_flags);
        case kRoundTowardPositive:
            return bid128_to_int32_ceil(dec128, &idec_signaling_flags);
        case kRoundTowardZero:
            return bid128_to_int32_int(dec128, &idec_signaling_flags);
        case kRoundTiesToAway:
            return bid128_to_int32_rninta(dec128, &idec_signaling_flags);
    }
    // Mimic behavior of Intel library (if round mode not valid, assume default)
    return bid128_to_int32_rnint(dec128, &idec_signaling_flags);
}

int64_t Decimal128::toLong(RoundingMode roundMode) {
    BID_UINT128 dec128 = decimal128ToLibraryType(_value);
    uint32_t idec_signaling_flags = 0;
    switch (roundMode) {
        case kRoundTiesToEven:
            return bid128_to_int64_rnint(dec128, &idec_signaling_flags);
        case kRoundTowardNegative:
            return bid128_to_int64_floor(dec128, &idec_signaling_flags);
        case kRoundTowardPositive:
            return bid128_to_int64_ceil(dec128, &idec_signaling_flags);
        case kRoundTowardZero:
            return bid128_to_int64_int(dec128, &idec_signaling_flags);
        case kRoundTiesToAway:
            return bid128_to_int64_rninta(dec128, &idec_signaling_flags);
    }
    // Mimic behavior of Intel library (if round mode not valid, assume default)
    return bid128_to_int64_rnint(dec128, &idec_signaling_flags);
}

double Decimal128::toDouble(RoundingMode roundMode) {
    // The Intel library float to float conversion always returns flags (not always for
    // integer types). Just discard them here.
    return isAndToDouble(roundMode).first;
}

std::string Decimal128::toString() {
    BID_UINT128 dec128 = decimal128ToLibraryType(_value);
    std::unique_ptr<char> c(new char());
    uint32_t idec_signaling_flags = 0;
    bid128_to_string(c.get(), dec128, &idec_signaling_flags);
    std::string s = c.get();
    return s;
}

std::pair<int32_t, bool> Decimal128::isAndToInt(RoundingMode roundMode) {
    BID_UINT128 dec128 = decimal128ToLibraryType(_value);
    uint32_t idec_signaling_flags = 0;
    switch (roundMode) {
        case kRoundTiesToEven:
            return std::make_pair<int32_t, bool>(
                bid128_to_int32_xrnint(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTowardNegative:
            return std::make_pair<int32_t, bool>(
                bid128_to_int32_xfloor(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTowardPositive:
            return std::make_pair<int32_t, bool>(
                bid128_to_int32_xceil(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTowardZero:
            return std::make_pair<int32_t, bool>(
                bid128_to_int32_xint(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTiesToAway:
            return std::make_pair<int32_t, bool>(
                bid128_to_int32_xrninta(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
    }
    return std::make_pair<int32_t, bool>(bid128_to_int32_xrnint(dec128, &idec_signaling_flags),
                                         idec_signaling_flags == 0);
}

std::pair<int64_t, bool> Decimal128::isAndToLong(RoundingMode roundMode) {
    BID_UINT128 dec128 = decimal128ToLibraryType(_value);
    uint32_t idec_signaling_flags = 0;
    switch (roundMode) {
        case kRoundTiesToEven:
            return std::make_pair<int64_t, bool>(
                bid128_to_int64_xrnint(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTowardNegative:
            return std::make_pair<int64_t, bool>(
                bid128_to_int64_xfloor(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTowardPositive:
            return std::make_pair<int64_t, bool>(
                bid128_to_int64_xceil(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTowardZero:
            return std::make_pair<int64_t, bool>(
                bid128_to_int64_xint(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
        case kRoundTiesToAway:
            return std::make_pair<int64_t, bool>(
                bid128_to_int64_xrninta(dec128, &idec_signaling_flags), idec_signaling_flags == 0);
    }
    // Mimic behavior of Intel library (if round mode not valid, assume default)
    return std::make_pair<int64_t, bool>(bid128_to_int64_xrnint(dec128, &idec_signaling_flags),
                                         idec_signaling_flags == 0);
}

std::pair<double, bool> Decimal128::isAndToDouble(RoundingMode roundMode) {
    BID_UINT128 dec128 = decimal128ToLibraryType(_value);
    uint32_t idec_signaling_flags = 0;
    return std::make_pair<double, bool>(
        bid128_to_binary64(dec128, roundMode, &idec_signaling_flags), idec_signaling_flags == 0);
}

bool Decimal128::isZero() {
    return bid128_isZero(decimal128ToLibraryType(_value));
}

bool Decimal128::isNaN() {
    return bid128_isNaN(decimal128ToLibraryType(_value));
}

bool Decimal128::isInfinite() {
    return bid128_isInf(decimal128ToLibraryType(_value));
}

bool Decimal128::isNegative() {
    return bid128_isSigned(decimal128ToLibraryType(_value));
}

Decimal128 Decimal128::add(const Decimal128& other, RoundingMode roundMode) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 addend = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_add(current, addend, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::subtract(const Decimal128& other, RoundingMode roundMode) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 sub = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_sub(current, sub, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::multiply(const Decimal128& other, RoundingMode roundMode) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 factor = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_mul(current, factor, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::divide(const Decimal128& other, RoundingMode roundMode) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 divisor = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_div(current, divisor, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::quantize(const Decimal128& reference, RoundingMode roundMode) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 q = decimal128ToLibraryType(reference.getValue());
    uint32_t idec_signaling_flags = 0;
    BID_UINT128 quantizedResult = bid128_quantize(current, q, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value;
    value.low64 = quantizedResult.w[LOW_64];
    value.high64 = quantizedResult.w[HIGH_64];
    Decimal128 result(value);
    return result;
}

bool Decimal128::isEqual(const Decimal128& other) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 compare = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_equal(current, compare, &idec_signaling_flags);
}

bool Decimal128::isNotEqual(const Decimal128& other) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 compare = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_not_equal(current, compare, &idec_signaling_flags);
}

bool Decimal128::isGreater(const Decimal128& other) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 compare = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_greater(current, compare, &idec_signaling_flags);
}

bool Decimal128::isGreaterEqual(const Decimal128& other) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 compare = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_greater_equal(current, compare, &idec_signaling_flags);
}

bool Decimal128::isLess(const Decimal128& other) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 compare = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_less(current, compare, &idec_signaling_flags);
}

bool Decimal128::isLessEqual(const Decimal128& other) {
    BID_UINT128 current = decimal128ToLibraryType(_value);
    BID_UINT128 compare = decimal128ToLibraryType(other.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_less_equal(current, compare, &idec_signaling_flags);
}

}  // namespace mongo
