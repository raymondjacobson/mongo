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
#include <iostream>

#include "mongo/platform/endian.h"

namespace mongo {

	// This helper function creates a library specific type for the
	// IntelRDFPMathLib20U1 library from Decimal128's _value
	BID_UINT128 Decimal128ToLibraryType(const unsigned long long* value) {
		BID_UINT128 dec128;
		dec128.w[0] = value[0];
		dec128.w[1] = value[1];
		return dec128;
	}

	Decimal128::Decimal128() {
		_value[0] = 0;
		_value[1] = 0;
		_idec_signaling_flags = 0;
	}
	Decimal128::Decimal128(int i) {
		BID_UINT128 dec128;
		dec128 = bid128_from_int32(i);
		_value[0] = dec128.w[0];
		_value[1] = dec128.w[1];
	}
	Decimal128::Decimal128(long l) {
		BID_UINT128 dec128;
		dec128 = bid128_from_int64(l);
		_value[0] = dec128.w[0];
		_value[1] = dec128.w[1];
	}
	// This function takes a double and constructs a Decimal128 object
	// given a roundMode with a fixed precision of 15. Doubles can only
	// properly represent a decimal precision of 15-17 digits.
	// The general idea is to quantize the direct double->dec128 conversion
	// with a quantum of 1E(-15 +/- base10 exponent equivalent of the double).
	// To do this, we find the smallest (abs value) base 10 exponent greater
	// than the double's base 2 exp and shift the quantizer's exp accordingly.
	Decimal128::Decimal128(double d, int roundMode) {
	// Determine system's endian ordering in order to construct decimal
	// 128 values directly (inexpensively)
#if MONGO_CONFIG_BYTE_ORDER == 1234
		int HIGH_64=1;
		int LOW_64=0;
#else
		int HIGH_64=0;
		int LOW_64=1;
#endif
		BID_UINT128 dec128;
		dec128 = binary64_to_bid128(d, roundMode, &_idec_signaling_flags);
		BID_UINT128 quantizerReference;
		// The quantizer starts at 1E-15 because a binary float's decimal
		// precision is necessarily >= 15
		quantizerReference.w[HIGH_64] = 0x3022000000000000;
		quantizerReference.w[LOW_64] = 0x0000000000000001;
		int exp;
		bool posExp = true;
		frexp(d, &exp); // Get the exponent from the incoming double
		if (exp < 0) {
			exp *= -1;
			posExp = false;
		}
		// Convert base 2 to base 10, ex: 2^7=128, 10^(7*.3)=125.89...
		// If, by chance, 10^(n*.3) < 2^n, we're at most 1 off, so add 1
		int base10Exp = (exp*3)/10;
		if (pow(10, base10Exp+1) < pow(2, exp)) {
			base10Exp += 1;
		}
		// Additionally increase the base10Exp by 1 because 10 = 10^1
		// whereas in the negative case 0.1 = 10^-2
		if (posExp) base10Exp += 1;
		BID_UINT128 base10ExpInBID; // Start with the representation of 1
		base10ExpInBID.w[HIGH_64] = 0x3040000000000000;
		base10ExpInBID.w[LOW_64] = 0x0000000000000001;
		// Scale the exponent by the base 10 exponent. This is necessary to keep
		// The precision of the quantizer reference correct. Different cohorts
		// behave differently as a quantizer reference.
		base10ExpInBID = bid128_scalbn(base10ExpInBID, base10Exp, 
			roundMode, &_idec_signaling_flags);
		// Multiply the quantizer by the base 10 exponent for the positive case
		// and divide for the negative one
		if (posExp) {
			quantizerReference = bid128_mul(quantizerReference, base10ExpInBID,
				roundMode, &_idec_signaling_flags);
		} else {
			quantizerReference = bid128_div(quantizerReference, base10ExpInBID,
				roundMode, &_idec_signaling_flags);
		}
		dec128 = bid128_quantize(dec128, quantizerReference, roundMode, &_idec_signaling_flags);
		_value[LOW_64] = dec128.w[LOW_64];
		_value[HIGH_64] = dec128.w[HIGH_64];
	}
	Decimal128::Decimal128(std::string s) {
		std::unique_ptr<char[]> charInput(new char[s.size() + 1]);
		std::copy(s.begin(), s.end(), charInput.get());
		charInput[s.size()] = '\0';
		BID_UINT128 dec128;
		dec128 = bid128_from_string(charInput.get(), 0, &_idec_signaling_flags);
		_value[0] = dec128.w[0];
		_value[1] = dec128.w[1];
	}
	Decimal128::~Decimal128() {}

	const unsigned long long* Decimal128::getValue() const {
		return _value;
	}
	void Decimal128::setValue(unsigned long long* ull) {
		_value[0] = ull[0];
		_value[1] = ull[1];
	}

	int Decimal128::toInt(int roundMode) {
		BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
		switch(roundMode) {
			case 0 : return bid128_to_int32_rnint(dec128, &_idec_signaling_flags);
			case 1 : return bid128_to_int32_floor(dec128, &_idec_signaling_flags);
			case 2 : return bid128_to_int32_ceil(dec128, &_idec_signaling_flags);
			case 3 : return bid128_to_int32_int(dec128, &_idec_signaling_flags);
			case 4 : return bid128_to_int32_rninta(dec128, &_idec_signaling_flags);
		}
		// Mimic behavior of Intel library (if round mode not valid, assume default)
		return bid128_to_int32_rnint(dec128, &_idec_signaling_flags);
	}
	long Decimal128::toLong(int roundMode) {
		BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
		switch(roundMode) {
			case 0 : return bid128_to_int64_rnint(dec128, &_idec_signaling_flags);
			case 1 : return bid128_to_int64_floor(dec128, &_idec_signaling_flags);
			case 2 : return bid128_to_int64_ceil(dec128, &_idec_signaling_flags);
			case 3 : return bid128_to_int64_int(dec128, &_idec_signaling_flags);
			case 4 : return bid128_to_int64_rninta(dec128, &_idec_signaling_flags);
		}
		// Mimic behavior of Intel library (if round mode not valid, assume default)
		return bid128_to_int64_rnint(dec128, &_idec_signaling_flags);
	}
	double Decimal128::toDouble(int roundMode) {
		BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
		return bid128_to_binary64(dec128, roundMode, &_idec_signaling_flags);
	}
	std::string Decimal128::toString() {
		BID_UINT128 dec128 = Decimal128ToLibraryType(_value);
		std::unique_ptr<char> c(new char());
		bid128_to_string(c.get(), dec128, &_idec_signaling_flags);
		std::string s = c.get();
		return s;
	}

	Decimal128 Decimal128::add(const Decimal128& dec128, int roundMode) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 addend = Decimal128ToLibraryType(dec128.getValue());
		current = bid128_add(current, addend, roundMode, &_idec_signaling_flags);
		Decimal128 result;
		result.setValue(current.w);
		return result;
	}
	Decimal128 Decimal128::subtract(const Decimal128& dec128, int roundMode) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 sub = Decimal128ToLibraryType(dec128.getValue());
		current = bid128_sub(current, sub, roundMode, &_idec_signaling_flags);
		Decimal128 result;
		result.setValue(current.w);
		return result;
	}
	Decimal128 Decimal128::multiply(const Decimal128& dec128, int roundMode) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 factor = Decimal128ToLibraryType(dec128.getValue());
		current = bid128_mul(current, factor, roundMode, &_idec_signaling_flags);
		Decimal128 result;
		result.setValue(current.w);
		return result;
	}
	Decimal128 Decimal128::divide(const Decimal128& dec128, int roundMode) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 divisor = Decimal128ToLibraryType(dec128.getValue());
		current = bid128_div(current, divisor, roundMode, &_idec_signaling_flags);
		Decimal128 result;
		result.setValue(current.w);
		return result;
	}
	Decimal128 Decimal128::quantize(const Decimal128& reference, int roundMode) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 q = Decimal128ToLibraryType(reference.getValue());
		BID_UINT128 quantizedResult = bid128_quantize(current, q, roundMode, &_idec_signaling_flags);
		Decimal128 result;
		result.setValue(quantizedResult.w);
		return result;
	}
	bool Decimal128::compareEqual(const Decimal128& dec128) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 compare = Decimal128ToLibraryType(dec128.getValue());
		return bid128_quiet_equal(current, compare, &_idec_signaling_flags);
	}
	bool Decimal128::compareNotEqual(const Decimal128& dec128) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 compare = Decimal128ToLibraryType(dec128.getValue());
		return bid128_quiet_not_equal(current, compare, &_idec_signaling_flags);
	}
	bool Decimal128::compareGreater(const Decimal128& dec128) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 compare = Decimal128ToLibraryType(dec128.getValue());
		return bid128_quiet_greater(current, compare, &_idec_signaling_flags);
	}
	bool Decimal128::compareGreaterEqual(const Decimal128& dec128) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 compare = Decimal128ToLibraryType(dec128.getValue());
		return bid128_quiet_greater_equal(current, compare, &_idec_signaling_flags);
	}
	bool Decimal128::compareLess(const Decimal128& dec128) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 compare = Decimal128ToLibraryType(dec128.getValue());
		return bid128_quiet_less(current, compare, &_idec_signaling_flags);
	}
	bool Decimal128::compareLessEqual(const Decimal128& dec128) {
		BID_UINT128 current = Decimal128ToLibraryType(_value);
		BID_UINT128 compare = Decimal128ToLibraryType(dec128.getValue());
		return bid128_quiet_less_equal(current, compare, &_idec_signaling_flags);
	}
}