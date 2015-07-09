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

#include "mongo/platform/decimal128.h"

#include <array>
#include <cmath>
#include <string>
#include <utility>

#include "mongo/config.h"
#include "mongo/platform/endian.h"
#include "mongo/stdx/memory.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

class Decimal128Test : public mongo::unittest::Test {
protected:
    int HIGH_64;
    int LOW_64;
    void setUp() {
#if MONGO_CONFIG_BYTE_ORDER == 1234
        HIGH_64 = 1;
        LOW_64 = 0;
#else
        HIGH_64 = 0;
        LOW_64 = 1;
#endif
    }
};

// Tests for Decimal128 constructors
TEST_F(Decimal128Test, TestDefaultConstructor) {
    Decimal128 d;
    Decimal128::Decimal128Value val = d.getValue();
    uint64_t ullZero = 0;
    ASSERT_EQUALS(val.high64, ullZero);
    ASSERT_EQUALS(val.low64, ullZero);
}

TEST_F(Decimal128Test, TestInt32ConstructorZero) {
    int32_t intZero = 0;
    Decimal128 d(intZero);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x3040000000000000 0000000000000000 = +0E+0
    uint64_t highBytes = std::stoull("3040000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("0000000000000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestInt32ConstructorMax) {
    int32_t intMax = 2147483647;
    Decimal128 d(intMax);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x3040000000000000 000000007fffffff = +2147483647E+0
    uint64_t highBytes = std::stoull("3040000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("000000007fffffff", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestInt32ConstructorMin) {
    int32_t intMin = -2147483648;
    Decimal128 d(intMin);
    Decimal128::Decimal128Value val = d.getValue();
    // 0xb040000000000000 000000007fffffff = -2147483648E+0
    uint64_t highBytes = std::stoull("b040000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("0000000080000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestInt64ConstructorZero) {
    int64_t longZero = 0;
    Decimal128 d(longZero);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x3040000000000000 0000000000000000 = +0E+0
    uint64_t highBytes = std::stoull("3040000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("0000000000000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestInt64ConstructorMax) {
    int64_t longMax = LONG_MAX;
    Decimal128 d(longMax);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x3040000000000000 7fffffffffffffff = +9223372036854775807E+0
    uint64_t highBytes = std::stoull("3040000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("7fffffffffffffff", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestInt64ConstructorMin) {
    int64_t longMin = LONG_MIN;
    Decimal128 d(longMin);
    Decimal128::Decimal128Value val = d.getValue();
    // 0xb040000000000000 8000000000000000 = -9223372036854775808E+0
    uint64_t highBytes = std::stoull("b040000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("8000000000000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant1) {
    double dbl = 0.1 / 10;
    Decimal128 d(dbl);
    Decimal128 e("0.01");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant2) {
    double dbl = 0.1 / 10000;
    Decimal128 d(dbl);
    Decimal128 e("0.00001");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant3) {
    double dbl = 0.1 / 1000 / 1000 / 1000 / 1000 / 1000 / 1000;
    Decimal128 d(dbl);
    Decimal128 e("1E-19");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant4) {
    double dbl = 0.01 * 1000 * 1000 * 1000 * 1000 * 1000 * 1000;
    Decimal128 d(dbl);
    Decimal128 e("100000000000000E+2");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant5) {
    double dbl = 0.0127;
    Decimal128 d(dbl);
    Decimal128 e("0.0127");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant6) {
    double dbl = 1234567890.12709;
    Decimal128 d(dbl);
    Decimal128 e("1234567890.12709");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuant7) {
    double dbl = 0.1129857 / 1000 / 1000 / 1000 / 1000 / 1000 / 1000;
    Decimal128 d(dbl);
    Decimal128 e("1.12985700000000E-19");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorQuantFailPoorLog10Of2Estimate) {
    double dbl = exp2(1000);
    Decimal128 d(dbl);
    Decimal128 e("1.07150860718627E301");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorZero) {
    double doubleZero = 0;
    Decimal128 d(doubleZero);
    Decimal128 e("0");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorNeg) {
    double doubleNeg = -1.0;
    Decimal128 d(doubleNeg);
    Decimal128 e("-1.0");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorMaxRoundDown) {
    double doubleMax = DBL_MAX;
    Decimal128 d(doubleMax, Decimal128::RoundingMode::kRoundTowardNegative);
    Decimal128 e("179769313486231E294");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorMaxRoundUp) {
    double doubleMax = DBL_MAX;
    Decimal128 d(doubleMax, Decimal128::RoundingMode::kRoundTowardPositive);
    Decimal128 e("179769313486232E294");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorMaxNeg) {
    double doubleMax = -1 * DBL_MAX;
    Decimal128 d(doubleMax);
    Decimal128 e("-179769313486232E294");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorMin) {
    double min = DBL_MIN;
    Decimal128 d(min);
    Decimal128 e("2.22507385850720E-308");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorMinNeg) {
    double min = -DBL_MIN;
    Decimal128 d(min);
    Decimal128 e("-2.22507385850720E-308");
    ASSERT_TRUE(d.isEqual(e));
}

TEST_F(Decimal128Test, TestDoubleConstructorInfinity) {
    double dbl = std::numeric_limits<double>::infinity();
    Decimal128 d(dbl);
    ASSERT_TRUE(d.isInfinite());
}

TEST_F(Decimal128Test, TestDoubleConstructorNaN) {
    double dbl = std::numeric_limits<double>::quiet_NaN();
    Decimal128 d(dbl);
    ASSERT_TRUE(d.isNaN());
}

TEST_F(Decimal128Test, TestStringConstructorInRange) {
    std::string s = "+2.010";
    Decimal128 d(s);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x303a000000000000 00000000000007da = +2.010
    uint64_t highBytes = std::stoull("303a000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("00000000000007da", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestStringConstructorPosInfinity) {
    std::string s = "+INFINITY";
    Decimal128 d(s);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x7800000000000000 0000000000000000 = +Inf
    uint64_t highBytes = std::stoull("7800000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("0000000000000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestStringConstructorNegInfinity) {
    std::string s = "-INFINITY";
    Decimal128 d(s);
    Decimal128::Decimal128Value val = d.getValue();
    // 0xf800000000000000 0000000000000000 = -Inf
    uint64_t highBytes = std::stoull("f800000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("0000000000000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}

TEST_F(Decimal128Test, TestStringConstructorNaN) {
    std::string s = "I am not a number!";
    Decimal128 d(s);
    Decimal128::Decimal128Value val = d.getValue();
    // 0x7c00000000000000 0000000000000000 = NaN
    uint64_t highBytes = std::stoull("7c00000000000000", nullptr, 16);
    uint64_t lowBytes = std::stoull("0000000000000000", nullptr, 16);
    ASSERT_EQUALS(val.high64, highBytes);
    ASSERT_EQUALS(val.low64, lowBytes);
}
// Tests for Decimal128 conversions
TEST_F(Decimal128Test, TestDecimal128ToInt32Even) {
    std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
    int32_t out[6] = {-3, -2, -2, 2, 2, 3};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toInt(), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt32Neg) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTowardNegative;
    std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
    int32_t out[6] = {-3, -3, -3, 2, 2, 2};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt32Pos) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTowardPositive;
    std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
    int32_t out[6] = {-2, -2, -2, 3, 3, 3};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt32Zero) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTowardZero;
    std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
    int32_t out[6] = {-2, -2, -2, 2, 2, 2};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt32Away) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTiesToAway;
    std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
    int32_t out[6] = {-3, -3, -2, 2, 3, 3};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt64Even) {
    std::string in[6] = {"-4294967296.7",
                         "-4294967296.5",
                         "-4294967296.2",
                         "4294967296.2",
                         "4294967296.5",
                         "4294967296.7"};
    int64_t out[6] = {-4294967297, -4294967296, -4294967296, 4294967296, 4294967296, 4294967297};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toLong(), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt64Neg) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTowardNegative;
    std::string in[6] = {"-4294967296.7",
                         "-4294967296.5",
                         "-4294967296.2",
                         "4294967296.2",
                         "4294967296.5",
                         "4294967296.7"};
    int64_t out[6] = {-4294967297, -4294967297, -4294967297, 4294967296, 4294967296, 4294967296};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt64Pos) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTowardPositive;
    std::string in[6] = {"-4294967296.7",
                         "-4294967296.5",
                         "-4294967296.2",
                         "4294967296.2",
                         "4294967296.5",
                         "4294967296.7"};
    int64_t out[6] = {-4294967296, -4294967296, -4294967296, 4294967297, 4294967297, 4294967297};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt64Zero) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTowardZero;
    std::string in[6] = {"-4294967296.7",
                         "-4294967296.5",
                         "-4294967296.2",
                         "4294967296.2",
                         "4294967296.5",
                         "4294967296.7"};
    int64_t out[6] = {-4294967296, -4294967296, -4294967296, 4294967296, 4294967296, 4294967296};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToInt64Away) {
    Decimal128::RoundingMode roundMode = Decimal128::RoundingMode::kRoundTiesToAway;
    std::string in[6] = {"-4294967296.7",
                         "-4294967296.5",
                         "-4294967296.2",
                         "4294967296.2",
                         "4294967296.5",
                         "4294967296.7"};
    int64_t out[6] = {-4294967297, -4294967297, -4294967296, 4294967296, 4294967297, 4294967297};
    std::unique_ptr<Decimal128> decPtr;
    for (int testNo = 0; testNo < 6; ++testNo) {
        decPtr = stdx::make_unique<Decimal128>(in[testNo]);
        ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
    }
}

TEST_F(Decimal128Test, TestDecimal128ToDoubleNormal) {
    std::string s = "+2.015";
    Decimal128 d(s);
    double result = d.toDouble();
    ASSERT_EQUALS(result, 2.015);
}

TEST_F(Decimal128Test, TestDecimal128ToDoubleZero) {
    std::string s = "+0.000";
    Decimal128 d(s);
    double result = d.toDouble();
    ASSERT_EQUALS(result, 0.0);
}

TEST_F(Decimal128Test, TestDecimal128ToStringPos) {
    std::string s = "2087.015E+281";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "2.087015E+284");
}

TEST_F(Decimal128Test, TestDecimal128ToStringNeg) {
    std::string s = "-2087.015E-281";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "-2.087015E-278");
}

TEST_F(Decimal128Test, TestDecimal128ToStringPosNaN) {
    std::string s = "+NaN";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "NaN");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeZero1) {
    std::string s = "0";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeZero2) {
    std::string s = "0.0";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.0");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeZero3) {
    std::string s = "0.00";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.00");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeZero4) {
    std::string s = "000.0";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.0");
}


TEST_F(Decimal128Test, TestDecimal128ToStringInRangeZero5) {
    std::string s = "0.000000000000";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0E-12");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangePos1) {
    std::string s = "1234567890.1234567890";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "1234567890.1234567890");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangePos2) {
    std::string s = "5.00";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "5.00");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangePos3) {
    std::string s = "50.0";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "50.0");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangePos4) {
    std::string s = "5";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "5");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangePos5) {
    std::string s = "50";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "50");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangePos5Minus) {
    std::string s = "-50";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "-50");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeNeg1) {
    std::string s = ".05";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.05");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeNeg2) {
    std::string s = ".5";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.5");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeNeg3) {
    std::string s = ".0052";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.0052");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeNeg4) {
    std::string s = ".005";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "0.005");
}

TEST_F(Decimal128Test, TestDecimal128ToStringInRangeNeg4Minus) {
    std::string s = "-.005";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "-0.005");
}

TEST_F(Decimal128Test, TestDecimal128ToStringOutRangeNeg1) {
    std::string s = ".0005";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "5E-4");
}

TEST_F(Decimal128Test, TestDecimal128ToStringOutRangeNeg2) {
    std::string s = ".000005123123123123";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "5.123123123123E-6");
}

TEST_F(Decimal128Test, TestDecimal128ToStringOutRangeNeg3) {
    std::string s = ".012587E-200";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "1.2587E-202");
}

TEST_F(Decimal128Test, TestDecimal128ToStringOutRangePos1) {
    std::string s = "1234567890123";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "1.234567890123E+12");
}

TEST_F(Decimal128Test, TestDecimal128ToStringOutRangePos2) {
    std::string s = "10201.01E14";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "1.020101E+18");
}

TEST_F(Decimal128Test, TestDecimal128ToStringOutRangePos3) {
    std::string s = "1234567890123456789012345678901234";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "1.234567890123456789012345678901234E+33");
}

TEST_F(Decimal128Test, TestDecimal128ToStringNegNaN) {
    std::string s = "-NaN";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "NaN");
}

TEST_F(Decimal128Test, TestDecimal128ToStringPosInf) {
    std::string s = "+Infinity";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "Inf");
}

TEST_F(Decimal128Test, TestDecimal128ToStringNegInf) {
    std::string s = "-NaN";
    Decimal128 d(s);
    std::string result = d.toString();
    ASSERT_EQUALS(result, "NaN");
}

// Tests for Decimal128 operations that use a signaling flag
TEST_F(Decimal128Test, TestDecimal128ToIntSignaling) {
    Decimal128 d("NaN");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    int32_t intVal = d.toInt(&sigFlag);
    ASSERT_EQUALS(intVal, std::numeric_limits<int32_t>::min());
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kInvalid);
}

TEST_F(Decimal128Test, TestDecimal128ToLongSignaling) {
    Decimal128 d("Infinity");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    int64_t longVal = d.toLong(&sigFlag);
    ASSERT_EQUALS(longVal, std::numeric_limits<int64_t>::lowest());
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kInvalid);
}

TEST_F(Decimal128Test, TestDecimal128ToIntExactSignaling) {
    Decimal128 d("10000000000000000");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    int32_t intVal = d.toInt(&sigFlag);
    ASSERT_EQUALS(intVal, -std::numeric_limits<int32_t>::lowest());
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kInvalid); //TODO
}

TEST_F(Decimal128Test, TestDecimal128ToLongExactSignaling) {
    Decimal128 d("100000000000000000000000000");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    int64_t longVal = d.toLong(&sigFlag);
    ASSERT_EQUALS(longVal, -std::numeric_limits<int64_t>::lowest());
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kInvalid); //TODO
}

TEST_F(Decimal128Test, TestDecimal128ToDoubleSignaling) {
    Decimal128 d("0.1");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    double doubleVal = d.toDouble(&sigFlag);
    ASSERT_EQUALS(doubleVal, 0.1);
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kInexact);
}

TEST_F(Decimal128Test, TestDecimal128AddSignaling) {
    Decimal128 d1("0.1");
    Decimal128 d2("0.1");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    d1.add(d2, &sigFlag);
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kNoFlag);
}

TEST_F(Decimal128Test, TestDecimal128SubtractSignaling) {
    Decimal128 d = Decimal128::getNegMin();
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    Decimal128 res = d.subtract(Decimal128(1), &sigFlag);
    ASSERT_TRUE(res.isEqual(Decimal128::getNegMin()));
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kInexact);
}

TEST_F(Decimal128Test, TestDecimal128MultiplySignaling) {
    Decimal128 d("2");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    Decimal128 res = d.multiply(Decimal128::getPosMax(), &sigFlag);
    ASSERT_TRUE(res.isEqual(Decimal128::getPosInfinity()));
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kOverflow);
}

TEST_F(Decimal128Test, TestDecimal128DivideSignaling) {
    Decimal128 d("2");
    uint32_t sigFlag = Decimal128::SignalingFlag::kNoFlag;
    Decimal128 res = d.divide(Decimal128(0), &sigFlag);
    ASSERT_TRUE(res.isEqual(Decimal128::getPosInfinity()));
    ASSERT_EQUALS(sigFlag, sigFlag | Decimal128::SignalingFlag::kDivideByZero);
}

// Test Decimal128 special comparisons
TEST_F(Decimal128Test, TestDecimal128IsZero) {
    Decimal128 d1(0);
    Decimal128 d2(500);
    ASSERT_TRUE(d1.isZero());
    ASSERT_FALSE(d2.isZero());
}

TEST_F(Decimal128Test, TestDecimal128IsNaN) {
    Decimal128 d1("NaN");
    Decimal128 d2("10.5");
    Decimal128 d3("Inf");
    ASSERT_TRUE(d1.isNaN());
    ASSERT_FALSE(d2.isNaN());
    ASSERT_FALSE(d3.isNaN());
}

TEST_F(Decimal128Test, TestDecimal128IsInfinite) {
    Decimal128 d1("NaN");
    Decimal128 d2("10.5");
    Decimal128 d3("Inf");
    Decimal128 d4("-Inf");
    ASSERT_FALSE(d1.isInfinite());
    ASSERT_FALSE(d2.isInfinite());
    ASSERT_TRUE(d3.isInfinite());
    ASSERT_TRUE(d4.isInfinite());
}

TEST_F(Decimal128Test, TestDecimal128IsNegative) {
    Decimal128 d1("NaN");
    Decimal128 d2("-NaN");
    Decimal128 d3("10.5");
    Decimal128 d4("-10.5");
    Decimal128 d5("Inf");
    Decimal128 d6("-Inf");
    ASSERT_FALSE(d1.isNegative());
    ASSERT_FALSE(d3.isNegative());
    ASSERT_FALSE(d5.isNegative());
    ASSERT_TRUE(d2.isNegative());
    ASSERT_TRUE(d4.isNegative());
    ASSERT_TRUE(d6.isNegative());
}

// Tests for Decimal128 math operations
TEST_F(Decimal128Test, TestDecimal128AdditionCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-50.5218E19");
    Decimal128 result = d1.add(d2);
    Decimal128 expected("1.999782E21");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128AdditionCase2) {
    Decimal128 d1("1.00");
    Decimal128 d2("2.000");
    Decimal128 result = d1.add(d2);
    Decimal128 expected("3.000");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128SubtractionCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-50.5218E19");
    Decimal128 result = d1.subtract(d2);
    Decimal128 expected("3.010218E21");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128SubtractionCase2) {
    Decimal128 d1("1.00");
    Decimal128 d2("2.000");
    Decimal128 result = d1.subtract(d2);
    Decimal128 expected("-1.000");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128MultiplicationCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-50.5218E19");
    Decimal128 result = d1.multiply(d2);
    Decimal128 expected("-1.265571090E42");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128MultiplicationCase2) {
    Decimal128 d1("1.00");
    Decimal128 d2("2.000");
    Decimal128 result = d1.multiply(d2);
    Decimal128 expected("2.00000");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128DivisionCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-50.5218E19");
    Decimal128 result = d1.divide(d2);
    Decimal128 expected("-4.958255644098191275845278671781290");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128DivisionCase2) {
    Decimal128 d1("1.00");
    Decimal128 d2("2.000");
    Decimal128 result = d1.divide(d2);
    Decimal128 expected("0.5");
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

TEST_F(Decimal128Test, TestDecimal128Quantizer) {
    Decimal128 expected("1.00001");
    Decimal128 val("1.000008");
    Decimal128 ref("0.00001");
    Decimal128 result = val.quantize(ref);
    ASSERT_EQUALS(result.getValue().low64, expected.getValue().low64);
    ASSERT_EQUALS(result.getValue().high64, expected.getValue().high64);
}

// Tests for Decimal128 comparison operations
TEST_F(Decimal128Test, TestDecimal128EqualCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("25.05E20");
    bool result = d1.isEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128EqualCase2) {
    Decimal128 d1("1.00");
    Decimal128 d2("1.000000000");
    bool result = d1.isEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128EqualCase3) {
    Decimal128 d1("0.1");
    Decimal128 d2("0.100000000000000005");
    bool result = d1.isEqual(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128EqualCase4) {
    Decimal128 d1("inf");
    Decimal128 d2("inf");
    bool result = d1.isEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128NotEqualCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("25.06E20");
    bool result = d1.isNotEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128NotEqualCase2) {
    Decimal128 d1("-25.0001E20");
    Decimal128 d2("-25.00010E20");
    bool result = d1.isNotEqual(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128GreaterCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-25.05E20");
    bool result = d1.isGreater(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128GreaterCase2) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("25.05E20");
    bool result = d1.isGreater(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128GreaterCase3) {
    Decimal128 d1("-INFINITY");
    Decimal128 d2("+INFINITY");
    bool result = d1.isGreater(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128GreaterEqualCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-25.05E20");
    bool result = d1.isGreaterEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128GreaterEqualCase2) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("25.05E20");
    bool result = d1.isGreaterEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128GreaterEqualCase3) {
    Decimal128 d1("-INFINITY");
    Decimal128 d2("+INFINITY");
    bool result = d1.isGreaterEqual(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128LessCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-25.05E20");
    bool result = d1.isLess(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128LessCase2) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("25.05E20");
    bool result = d1.isLess(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128LessCase3) {
    Decimal128 d1("-INFINITY");
    Decimal128 d2("+INFINITY");
    bool result = d1.isLess(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128LessEqualCase1) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("-25.05E20");
    bool result = d1.isLessEqual(d2);
    ASSERT_FALSE(result);
}

TEST_F(Decimal128Test, TestDecimal128LessEqualCase2) {
    Decimal128 d1("25.05E20");
    Decimal128 d2("25.05E20");
    bool result = d1.isLessEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128LessEqualCase3) {
    Decimal128 d1("-INFINITY");
    Decimal128 d2("+INFINITY");
    bool result = d1.isLessEqual(d2);
    ASSERT_TRUE(result);
}

TEST_F(Decimal128Test, TestDecimal128GetPosMin) {
    Decimal128 d = Decimal128::getPosMin();
    uint64_t high = 0;
    uint64_t low = 1;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetPosMax) {
    Decimal128 d = Decimal128::getPosMax();
    uint64_t high = 6917508178773903296ull;
    uint64_t low = 4003012203950112767ull;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetNegMin) {
    Decimal128 d = Decimal128::getNegMin();
    uint64_t high = 16140880215628679104ull;
    uint64_t low = 4003012203950112767ull;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetNegMax) {
    Decimal128 d = Decimal128::getNegMax();
    uint64_t high = 9223372036854775808ull;
    uint64_t low = 1ull;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetPosInfinity) {
    Decimal128 d = Decimal128::getPosInfinity();
    uint64_t high = 8646911284551352320ull;
    uint64_t low = 0;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetNegInfinity) {
    Decimal128 d = Decimal128::getNegInfinity();
    uint64_t high = 17870283321406128128ull;
    uint64_t low = 0;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetPosNaN) {
    Decimal128 d = Decimal128::getPosNaN();
    uint64_t high = 8935141660703064064ull;
    uint64_t low = 0;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

TEST_F(Decimal128Test, TestDecimal128GetNegNaN) {
    Decimal128 d = Decimal128::getNegNaN();
    uint64_t high = 18158513697557839872ull;
    uint64_t low = 0;
    ASSERT_EQUALS(d.getValue().high64, high);
    ASSERT_EQUALS(d.getValue().low64, low);
}

}  // namespace mongo
