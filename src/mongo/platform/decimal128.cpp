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

#include <math.h>
#include <memory>
#include <string>
#include <utility>

#include "mongo/platform/endian.h"

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
BID_UINT128 Decimal128ToLibraryType(Decimal128::Decimal128Value value) {
    BID_UINT128 dec128;
    dec128.w[LOW_64] = value.low64;
    dec128.w[HIGH_64] = value.high64;
    return dec128;
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
    BID_UINT128 dec128;
    uint32_t idec_signaling_flags = 0;
    dec128 = binary64_to_bid128(doubleValue, roundMode, &idec_signaling_flags);
    BID_UINT128 quantizerReference;
    // The quantizer starts at 1E-15 because a binary float's decimal
    // precision is necessarily >= 15
    quantizerReference.w[HIGH_64] = 0x3022000000000000;
    quantizerReference.w[LOW_64] = 0x0000000000000001;
    int exp;
    bool posExp = true;
    frexp(doubleValue, &exp);  // Get the exponent from the incoming double
    if (exp < 0) {
        exp *= -1;
        posExp = false;
    }
    // Convert base 2 to base 10, ex: 2^7=128, 10^(7*.3)=125.89...
    // If, by chance, 10^(n*.3) < 2^n, we're at most 1 off, so add 1
    int base10Exp = (exp * 3) / 10;
    if (pow(10, base10Exp + 1) < pow(2, exp)) {
        base10Exp += 1;
    }
    // Additionally increase the base10Exp by 1 because 10 = 10^1
    // whereas in the negative case 0.1 = 10^-2
    if (posExp)
        base10Exp += 1;
    BID_UINT128 base10ExpInBID;  // Start with the representation of 1
    base10ExpInBID.w[HIGH_64] = 0x3040000000000000;
    base10ExpInBID.w[LOW_64] = 0x0000000000000001;
    // Scale the exponent by the base 10 exponent. This is necessary to keep
    // The precision of the quantizer reference correct. Different cohorts
    // behave differently as a quantizer reference.
    base10ExpInBID = bid128_scalbn(base10ExpInBID, base10Exp, roundMode, &idec_signaling_flags);
    // Multiply the quantizer by the base 10 exponent for the positive case
    // and divide for the negative one
    if (posExp) {
        quantizerReference =
            bid128_mul(quantizerReference, base10ExpInBID, roundMode, &idec_signaling_flags);
    } else {
        quantizerReference =
            bid128_div(quantizerReference, base10ExpInBID, roundMode, &idec_signaling_flags);
    }
    dec128 = bid128_quantize(dec128, quantizerReference, roundMode, &idec_signaling_flags);
    _value = Decimal128Value(dec128.w);
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
    BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
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
    BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
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
    BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
    std::unique_ptr<char> c(new char());
    uint32_t idec_signaling_flags = 0;
    bid128_to_string(c.get(), dec128, &idec_signaling_flags);
    std::string s = c.get();
    return s;
}

std::pair<int32_t, bool> Decimal128::isAndToInt(RoundingMode roundMode) {
    BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
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
    BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
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
    BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
    uint32_t idec_signaling_flags = 0;
    return std::make_pair<double, bool>(
        bid128_to_binary64(dec128, roundMode, &idec_signaling_flags), idec_signaling_flags == 0);
}

bool Decimal128::isNaN() {
    return bid128_isNaN(Decimal128ToLibraryType(_value));
}

bool Decimal128::isInfinite() {
    return bid128_isInf(Decimal128ToLibraryType(_value));
}

bool Decimal128::isNegative() {
    return bid128_isSigned(Decimal128ToLibraryType(_value));
}

Decimal128 Decimal128::add(const Decimal128& dec128, RoundingMode roundMode) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 addend = Decimal128ToLibraryType(dec128.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_add(current, addend, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::subtract(const Decimal128& rhs, RoundingMode roundMode) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 sub = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_sub(current, sub, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::multiply(const Decimal128& rhs, RoundingMode roundMode) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 factor = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_mul(current, factor, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::divide(const Decimal128& rhs, RoundingMode roundMode) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 divisor = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    current = bid128_div(current, divisor, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value(current.w);
    Decimal128 result(value);
    return result;
}

Decimal128 Decimal128::quantize(const Decimal128& reference, RoundingMode roundMode) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 q = Decimal128ToLibraryType(reference.getValue());
    uint32_t idec_signaling_flags = 0;
    BID_UINT128 quantizedResult = bid128_quantize(current, q, roundMode, &idec_signaling_flags);
    Decimal128::Decimal128Value value;
    value.low64 = quantizedResult.w[LOW_64];
    value.high64 = quantizedResult.w[HIGH_64];
    Decimal128 result(value);
    return result;
}

bool Decimal128::isEqual(const Decimal128& rhs) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 compare = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_equal(current, compare, &idec_signaling_flags);
}

bool Decimal128::isNotEqual(const Decimal128& rhs) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 compare = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_not_equal(current, compare, &idec_signaling_flags);
}

bool Decimal128::isGreater(const Decimal128& rhs) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 compare = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_greater(current, compare, &idec_signaling_flags);
}

bool Decimal128::isGreaterEqual(const Decimal128& rhs) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 compare = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_greater_equal(current, compare, &idec_signaling_flags);
}

bool Decimal128::isLess(const Decimal128& rhs) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 compare = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_less(current, compare, &idec_signaling_flags);
}

bool Decimal128::isLessEqual(const Decimal128& rhs) {
    BID_UINT128 current = Decimal128ToLibraryType(_value);
    BID_UINT128 compare = Decimal128ToLibraryType(rhs.getValue());
    uint32_t idec_signaling_flags = 0;
    return bid128_quiet_less_equal(current, compare, &idec_signaling_flags);
}

}  // namespace mongo