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
    // Don't quantize if infinity or NaN, just set the _value and return
    if (bid128_isInf(dec128) || bid128_isNaN(dec128)) {
        _value = Decimal128Value(dec128.w);
        return;
    }
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

const Decimal128::Decimal128Value& Decimal128::getValue() const {
    return _value;
}

Decimal128::Decimal128Value& Decimal128::getValue() {
    return const_cast<Decimal128::Decimal128Value&>(
        static_cast<const Decimal128::Decimal128&>(*this).getValue());
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
    std::unique_ptr<char[]> c(
        new char[1 /* mantissa sign */ + 34 /* mantissa */ + 1 /* scientific E */ +
                 1 /* exponent sign */ + 4 /* exponent */ + 1 /* null terminator */]);
    uint32_t idec_signaling_flags = 0;
    bid128_to_string(c.get(), dec128, &idec_signaling_flags);

    std::string s = c.get();
    int precision = 0;
    int exponent = 0;

    // Deal with a NaN and Infinity
    std::string::size_type ePos = s.find("E");
    if (ePos == std::string::npos) {
        if (s == "-NaN" || s == "+NaN")
            return "NaN";
        if (s[0] == '+')
            return "Inf";
        return s;
    }

    std::string exponentString = s.substr(ePos);
    bool posExp = true;
    if (exponentString[1] == '-') {
        posExp = false;
    }

    // Get the value of the exponent, start at 2 to ignore the E and the sign
    for (std::string::size_type i = 2; i < exponentString.size(); ++i) {
        exponent = exponent * 10 + (exponentString[i] - '0');
    }
    // Get the total precision of the number
    precision = s.size() - exponentString.size() - 1 /* mantissa sign */;

    int exponentAdjusted = precision - exponent;
    std::string res;

    // First, check if the exponent is easily represented without scientific notation
    // Otherwise, properly convert to scientific notation (normalized)
    if (exponentAdjusted <= 12 && exponentAdjusted > 0) {
        res = s.substr(0, exponentAdjusted + 1); // Add everything before the decimal point
        // If the exponent was zero, we would not need anything after the decimal point
        if (exponent != 0) {
            res += ".";
            res += s.substr(exponentAdjusted + 1, ePos - (exponentAdjusted + 1));
        }
    } else if (exponentAdjusted >= -4 && exponentAdjusted <= 0) {
        res += s.substr(0, 1) + "0."; // Add the sign and prefix zero
        for (int i = 0; i > exponentAdjusted; --i) {
            res += '0';  // Add leading zeros
        }
        res += s.substr(1, precision);
    } else {
        res = s.substr(0, 2);  // Sign + 1st digit
        if (precision > 1)
            res += ".";
        res += (s.substr(2, precision - 1) + "E");
        exponentAdjusted = -exponentAdjusted + 1;
        if (exponentAdjusted < 0)
            exponentAdjusted *= -1;
        if (posExp)
            res += "+";
        else
            res += "-";
        res += std::to_string(exponentAdjusted);
    }
    // Remove the leading '+' if the number is positive
    if (res[0] == '+') {
        res.erase(0, 1);
    }
    return res;
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

bool Decimal128::isZero() {
    return bid128_isZero(Decimal128ToLibraryType(_value));
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

Decimal128 Decimal128::getPosMin() {
    uint64_t val[2];
    val[HIGH_64] = 0ull;
    val[LOW_64] = 1ull;
    Decimal128 min((Decimal128Value::Decimal128Value(val)));
    return min;
}

Decimal128 Decimal128::getPosMax() {
    uint64_t val[2];
    val[HIGH_64] = 6917508178773903296ull;
    val[LOW_64] = 4003012203950112767ull;
    Decimal128 max((Decimal128Value::Decimal128Value(val)));
    return max;
}

Decimal128 Decimal128::getNegMin() {
    uint64_t val[2];
    val[HIGH_64] = 16140880215628679104ull;
    val[LOW_64] = 4003012203950112767ull;
    Decimal128 min((Decimal128Value::Decimal128Value(val)));
    return min;
}

Decimal128 Decimal128::getNegMax() {
    uint64_t val[2];
    val[HIGH_64] = 9223372036854775808ull;
    val[LOW_64] = 1ull;
    Decimal128 max((Decimal128Value::Decimal128Value(val)));
    return max;
}

Decimal128 Decimal128::getPosInfinity() {
    uint64_t val[2];
    val[HIGH_64] = 8646911284551352320ull;
    val[LOW_64] = 0ull;
    Decimal128 posInf((Decimal128Value::Decimal128Value(val)));
    return posInf;
}

Decimal128 Decimal128::getNegInfinity() {
    uint64_t val[2];
    val[HIGH_64] = 17870283321406128128ull;
    val[LOW_64] = 0ull;
    Decimal128 negInf((Decimal128Value::Decimal128Value(val)));
    return negInf;
}

Decimal128 Decimal128::getPosNaN() {
    uint64_t val[2];
    val[HIGH_64] = 8935141660703064064ull;
    val[LOW_64] = 0ull;
    Decimal128 posNaN((Decimal128Value::Decimal128Value(val)));
    return posNaN;
}

Decimal128 Decimal128::getNegNaN() {
    uint64_t val[2];
    val[HIGH_64] = 18158513697557839872ull;
    val[LOW_64] = 0ull;
    Decimal128 negNaN((Decimal128Value::Decimal128Value(val)));
    return negNaN;
}

}  // namespace mongo