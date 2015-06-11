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

#include <memory>
#include <string>

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
	Decimal128::Decimal128(double d, int roundMode) {
		BID_UINT128 dec128;
		dec128 = binary64_to_bid128(d, roundMode, &_idec_signaling_flags);
		_value[0] = dec128.w[0];
		_value[1] = dec128.w[1];
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