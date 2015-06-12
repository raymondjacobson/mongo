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

#include <iostream>
#include <string>
#include <third_party/IntelRDFPMathLib20U1/LIBRARY/src/bid_conf.h>
#include <third_party/IntelRDFPMathLib20U1/LIBRARY/src/bid_functions.h>

namespace mongo {
// Wrapper class for Intel Decimal128 data type. Sample usage:
//     Decimal128 d("+10.0");
//     std::cout << d.toString << std::endl;
// Round modes:
// 	   0 - Round ties to even (default)
//     1 - Round toward negative/downward
//     2 - Round toward positive/upward
//     3 - Round toward zero
//     4 - Round ties to away
class Decimal128 {
public:
	Decimal128();
	Decimal128(int i);
	Decimal128(long l);
	Decimal128(double d, int roundMode = 0);
	Decimal128(std::string s);
	~Decimal128();

	// These functions get and set the two 64 bit arrays storing the
	// decimal128 value, which is useful for direct manipulation and testing
	const unsigned long long* getValue() const;
	void setValue(unsigned long long* ull);
	
	// Conversion to other types
	int toInt(int roundMode = 0);
	long toLong(int roundMode = 0);
	// This function constructs a decimal128 value from a double
	// and fixes the precision to 15, which is a binary double's
	// max guaranteed decimal precision.
	double toDouble(int roundMode = 0);
	std::string toString();

	// Mathematical operations
	Decimal128 add(const Decimal128& dec128, int roundMode=0);
	Decimal128 subtract(const Decimal128& dec128, int roundMode=0);
	Decimal128 multiply(const Decimal128& dec128, int roundMode=0);
	Decimal128 divide(const Decimal128& dec128, int roundMode=0);
	// This function quantizes the current decimal given a quantum reference
	Decimal128 quantize(const Decimal128& reference, int roundMode=0);

	// Comparison operations
	bool compareEqual(const Decimal128& dec128);
	bool compareNotEqual(const Decimal128& dec128);
	bool compareGreater(const Decimal128& dec128);
	bool compareGreaterEqual(const Decimal128& dec128);
	bool compareLess(const Decimal128& dec128);
	bool compareLessEqual(const Decimal128& dec128);

private:
	unsigned long long _value[2];
	// IDEC signaling flags are used for signaling operations.
	// Flags are passed to and modified in library calls
	unsigned int _idec_signaling_flags;
};

}