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

#include <iostream>
#include <string>

#include "mongo/platform/endian.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

class Decimal128Test : public mongo::unittest::Test {
protected:
	int HIGH_64;
	int LOW_64;
	void setUp() {
#if MONGO_CONFIG_BYTE_ORDER == 1234
		HIGH_64=1;
		LOW_64=0;
#else
		HIGH_64=0;
		LOW_64=1;
#endif
    }
};

	// Tests for Decimal128 constructors
	TEST_F(Decimal128Test, TestDefaultConstructor) {
		Decimal128 d;
		const unsigned long long* val = d.getValue();
		unsigned long long ullZero = 0;
		ASSERT_EQUALS(val[1], ullZero);
		ASSERT_EQUALS(val[0], ullZero);
	}
	TEST_F(Decimal128Test, TestInt32ConstructorZero) {
		int intZero = 0;
		Decimal128 d(intZero);
		const unsigned long long* val = d.getValue();
		// 0x3040000000000000 0000000000000000 = +0E+0
		unsigned long long highBytes = std::stoull("3040000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("0000000000000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestInt32ConstructorMax) {
		int intMax = INT_MAX;
		Decimal128 d(intMax);
		const unsigned long long* val = d.getValue();
		// 0x3040000000000000 000000007fffffff = +2147483647E+0
		unsigned long long highBytes = std::stoull("3040000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("000000007fffffff", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestInt32ConstructorMin) {
		int intMin = INT_MIN;
		Decimal128 d(intMin);
		const unsigned long long* val = d.getValue();
		// 0xb040000000000000 000000007fffffff = -2147483648E+0
		unsigned long long highBytes = std::stoull("b040000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("0000000080000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestInt64ConstructorZero) {
		long longZero = 0;
		Decimal128 d(longZero);
		const unsigned long long* val = d.getValue();
		// 0x3040000000000000 0000000000000000 = +0E+0
		unsigned long long highBytes = std::stoull("3040000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("0000000000000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestInt64ConstructorMax) {
		long longMax = LONG_MAX;
		Decimal128 d(longMax);
		const unsigned long long* val = d.getValue();
		// 0x3040000000000000 7fffffffffffffff = +9223372036854775807E+0
		unsigned long long highBytes = std::stoull("3040000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("7fffffffffffffff", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestInt64ConstructorMin) {
		long longMin = LONG_MIN;
		Decimal128 d(longMin);
		const unsigned long long* val = d.getValue();
		// 0xb040000000000000 8000000000000000 = -9223372036854775808E+0
		unsigned long long highBytes = std::stoull("b040000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("8000000000000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant1) {
		double dbl = 0.1/10;
		Decimal128 d(dbl);
		Decimal128 e("0.01");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant2) {
		double dbl = 0.1/10000;
		Decimal128 d(dbl);
		// std::cout<<d.toString()<<std::endl;
		Decimal128 e("0.00001");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant3) {
		double dbl = 0.1/1000/1000/1000/1000/1000/1000;
		Decimal128 d(dbl);
		Decimal128 e("1E-19");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant4) {
		double dbl = 0.01*1000*1000*1000*1000*1000*1000;
		Decimal128 d(dbl);
		Decimal128 e("100000000000000E+2");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant5) {
		double dbl = 0.0127;
		Decimal128 d(dbl);
		Decimal128 e("0.0127");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant6) {
		double dbl = 1234567890.12709;
		Decimal128 d(dbl);
		Decimal128 e("1234567890.12709");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorQuant7) {
		double dbl = 0.1129857/1000/1000/1000/1000/1000/1000;
		Decimal128 d(dbl);
		Decimal128 e("1.12985700000000E-19");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorZero) {
		double doubleZero = 0;
		Decimal128 d(doubleZero);
		Decimal128 e("0");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorMaxRoundDown) {
		double doubleMax = DBL_MAX;
		Decimal128 d(doubleMax, 1);
		Decimal128 e("179769313486231E294");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorMaxRoundUp) {
		double doubleMax = DBL_MAX;
		Decimal128 d(doubleMax, 2);
		Decimal128 e("179769313486232E294");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorMaxNeg) {
		double doubleMax = -1*DBL_MAX;
		Decimal128 d(doubleMax);
		Decimal128 e("-179769313486232E294");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorMin) {
		double min = DBL_MIN;
		Decimal128 d(min);
		Decimal128 e("2.22507385850720E-308");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestDoubleConstructorMinNeg) {
		double min = -DBL_MIN;
		Decimal128 d(min);
		Decimal128 e("-2.22507385850720E-308");
		ASSERT_TRUE(d.compareEqual(e));
	}
	TEST_F(Decimal128Test, TestStringConstructorInRange) {
		std::string s = "+2.010";
		Decimal128 d(s);
		const unsigned long long* val = d.getValue();
		// 0x303a000000000000 00000000000007da = +2.010
		unsigned long long highBytes = std::stoull("303a000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("00000000000007da", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestStringConstructorPosInfinity) {
		std::string s = "+INFINITY";
		Decimal128 d(s);
		const unsigned long long* val = d.getValue();
		// 0x7800000000000000 0000000000000000 = +Inf
		unsigned long long highBytes = std::stoull("7800000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("0000000000000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestStringConstructorNegInfinity) {
		std::string s = "-INFINITY";
		Decimal128 d(s);
		const unsigned long long* val = d.getValue();
		// 0xf800000000000000 0000000000000000 = -Inf
		unsigned long long highBytes = std::stoull("f800000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("0000000000000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	TEST_F(Decimal128Test, TestStringConstructorNaN) {
		std::string s = "I am not a number!";
		Decimal128 d(s);
		const unsigned long long* val = d.getValue();
		// 0x7c00000000000000 0000000000000000 = NaN
		unsigned long long highBytes = std::stoull("7c00000000000000", nullptr, 16);
		unsigned long long lowBytes = std::stoull("0000000000000000", nullptr, 16);
		ASSERT_EQUALS(val[HIGH_64], highBytes);
		ASSERT_EQUALS(val[LOW_64], lowBytes);
	}
	// Tests for Decimal128 conversions
	TEST_F(Decimal128Test, TestDecimal128ToInt32Even) {
		std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
		int out[6] = {-3, -2, -2, 2, 2, 3};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toInt(), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt32Neg) {
		int roundMode = 1;
		std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
		int out[6] = {-3, -3, -3, 2, 2, 2};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt32Pos) {
		int roundMode = 2;
		std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
		int out[6] = {-2, -2, -2, 3, 3, 3};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt32Zero) {
		int roundMode = 3;
		std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
		int out[6] = {-2, -2, -2, 2, 2, 2};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt32Away) {
		int roundMode = 4;
		std::string in[6] = {"-2.7", "-2.5", "-2.2", "2.2", "2.5", "2.7"};
		int out[6] = {-3, -3, -2, 2, 3, 3};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toInt(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt64Even) {
		std::string in[6] = {"-4294967296.7", "-4294967296.5", "-4294967296.2",
			"4294967296.2", "4294967296.5", "4294967296.7"};
		long out[6] = {-4294967297, -4294967296, -4294967296,
			4294967296, 4294967296, 4294967297};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toLong(), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt64Neg) {
		int roundMode = 1;
		std::string in[6] = {"-4294967296.7", "-4294967296.5", "-4294967296.2",
			"4294967296.2", "4294967296.5", "4294967296.7"};
		long out[6] = {-4294967297, -4294967297, -4294967297,
			4294967296, 4294967296, 4294967296};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt64Pos) {
		int roundMode = 2;
		std::string in[6] = {"-4294967296.7", "-4294967296.5", "-4294967296.2",
			"4294967296.2", "4294967296.5", "4294967296.7"};
		long out[6] = {-4294967296, -4294967296, -4294967296,
			4294967297, 4294967297, 4294967297};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt64Zero) {
		int roundMode = 3;
		std::string in[6] = {"-4294967296.7", "-4294967296.5", "-4294967296.2",
			"4294967296.2", "4294967296.5", "4294967296.7"};
		long out[6] = {-4294967296, -4294967296, -4294967296,
			4294967296, 4294967296, 4294967296};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
			ASSERT_EQUALS(decPtr->toLong(roundMode), out[testNo]);
		}
	}
	TEST_F(Decimal128Test, TestDecimal128ToInt64Away) {
		int roundMode = 4;
		std::string in[6] = {"-4294967296.7", "-4294967296.5", "-4294967296.2",
			"4294967296.2", "4294967296.5", "4294967296.7"};
		long out[6] = {-4294967297, -4294967297, -4294967296,
			4294967296, 4294967297, 4294967297};
		boost::scoped_ptr<Decimal128> decPtr;
		for (int testNo=0; testNo<6; ++testNo) {
			decPtr.reset(new Decimal128(in[testNo]));
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
	TEST_F(Decimal128Test, TestDecimal128ToString) {
		std::string s = "-2087.015E+281";
		Decimal128 d(s);
		std::string result = d.toString();
		ASSERT_EQUALS(result, "-2087015E+278");
	}

	// Tests for Decimal128 math operations
	TEST_F(Decimal128Test, TestDecimal128AdditionCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-50.5218E19";
		std::string s3 = "1.999782E21";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.add(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128AdditionCase2) {
		std::string s1 = "1.00";
		std::string s2 = "2.000";
		std::string s3 = "3.000";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.add(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128SubtractionCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-50.5218E19";
		std::string s3 = "3.010218E21";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.subtract(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128SubtractionCase2) {
		std::string s1 = "1.00";
		std::string s2 = "2.000";
		std::string s3 = "-1.000";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.subtract(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128MultiplicationCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-50.5218E19";
		std::string s3 = "-1.265571090E42";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.multiply(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128MultiplicationCase2) {
		std::string s1 = "1.00";
		std::string s2 = "2.000";
		std::string s3 = "2.00000";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.multiply(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128DivisionCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-50.5218E19";
		std::string s3 = "-4.958255644098191275845278671781290";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.divide(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128DivisionCase2) {
		std::string s1 = "1.00";
		std::string s2 = "2.000";
		std::string s3 = "0.5";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		Decimal128 result = d1.divide(d2);
		Decimal128 expected(s3);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}
	TEST_F(Decimal128Test, TestDecimal128Quantizer) {
		std::string s1 = "1.000008";
		std::string s2 = "0.00001";
		std::string s3 = "1.00001";
		Decimal128 expected(s3);
		Decimal128 val(s1);
		Decimal128 ref(s2);
		Decimal128 result = val.quantize(ref);
		ASSERT_EQUALS(result.getValue()[0], expected.getValue()[0]);
		ASSERT_EQUALS(result.getValue()[1], expected.getValue()[1]);
	}

	// Tests for Decimal128 comparison operations
	TEST_F(Decimal128Test, TestDecimal128EqualCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareEqual(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128EqualCase2) {
		std::string s1 = "1.00";
		std::string s2 = "1.000000000";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareEqual(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128EqualCase3) {
		std::string s1 = "0.1";
		std::string s2 = "0.100000000000000005";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareEqual(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128NotEqualCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "25.06E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareNotEqual(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128NotEqualCase2) {
		std::string s1 = "-25.0001E20";
		std::string s2 = "-25.00010E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareNotEqual(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128GreaterCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareGreater(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128GreaterCase2) {
		std::string s1 = "25.05E20";
		std::string s2 = "25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareGreater(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128GreaterCase3) {
		std::string s1 = "-INFINITY";
		std::string s2 = "+INFINITY";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareGreater(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128GreaterEqualCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareGreaterEqual(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128GreaterEqualCase2) {
		std::string s1 = "25.05E20";
		std::string s2 = "25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareGreaterEqual(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128GreaterEqualCase3) {
		std::string s1 = "-INFINITY";
		std::string s2 = "+INFINITY";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareGreaterEqual(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128LessCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareLess(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128LessCase2) {
		std::string s1 = "25.05E20";
		std::string s2 = "25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareLess(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128LessCase3) {
		std::string s1 = "-INFINITY";
		std::string s2 = "+INFINITY";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareLess(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128LessEqualCase1) {
		std::string s1 = "25.05E20";
		std::string s2 = "-25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareLessEqual(d2);
		ASSERT_FALSE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128LessEqualCase2) {
		std::string s1 = "25.05E20";
		std::string s2 = "25.05E20";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareLessEqual(d2);
		ASSERT_TRUE(result);
	}
	TEST_F(Decimal128Test, TestDecimal128LessEqualCase3) {
		std::string s1 = "-INFINITY";
		std::string s2 = "+INFINITY";
		Decimal128 d1(s1);
		Decimal128 d2(s2);
		bool result = d1.compareLessEqual(d2);
		ASSERT_TRUE(result);
	}

}