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

#include "third_party/IntelRDFPMathLib20U1/LIBRARY/src/bid_conf.h"
#include "third_party/IntelRDFPMathLib20U1/LIBRARY/src/bid_functions.h"

#include <string>
#include <iostream>

namespace mongo {

	class IntelDecimal128 {
	public:
		IntelDecimal128();
		~IntelDecimal128();

		// // String and IO wrappers
		// std::string decimal128ToString();

		// // Mathematical operations wrappers
		// decimal128 decimal128Add(decimal128 dec128);
		// decimal128 decimal128Subtract(decimal128 dec128);
		// decimal128 decimal128Multiply(decimal128 dec128);
		// decimal128 decimal128Divide(decimal128 dec128);

		// // Conversion to and from other types wrappers
		// decimal128 decimal128FromInt(int i);
		// int decimal128ToInt();
		// decimal128 decimal128FromLong(long i);
		// long decimal128ToLong();
		// decimal128 decimal128FromDouble(double i);
		// double decimal128ToDouble();

		// // Comparason wrappers
		// bool decimal128CompareEqual(decimal128 dec128);
		// bool decimal128CompareNotEqual(decimal128 dec128);
		// bool decimal128CompareGreater(decimal128 dec128);
		// bool decimal128CompareGreaterEqual(decimal128 dec128);
		// bool decimal128CompareLess(decimal128 dec128);
		// bool decimal128CompareLessEqual(decimal128 dec128);

		// Operator overloads
		friend std::ostream& operator<<(std::ostream& ostr, const IntelDecimal128& dec128);

	private:
		unsigned int _idec_signaling_flags;
	};
}