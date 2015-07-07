/**
 * Copyright (C) 2015 MongoDB Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the copyright holders give permission to link the
 * code of portions of this program with the OpenSSL library under certain
 * conditions as described in each individual source file and distribute
 * linked combinations including the program with the OpenSSL library. You
 * must comply with the GNU Affero General Public License in all respects
 * for all of the code used other than as permitted herein. If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so. If you do not
 * wish to do so, delete this exception statement from your version. If you
 * delete this exception statement from all source files in the program,
 * then also delete it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/scripting/mozjs/numberdecimal.h"

#include "mongo/platform/decimal128.h"
#include "mongo/scripting/mozjs/implscope.h"
#include "mongo/scripting/mozjs/objectwrapper.h"
#include "mongo/scripting/mozjs/valuereader.h"
#include "mongo/scripting/mozjs/valuewriter.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/text.h"

namespace mongo {
namespace mozjs {

const JSFunctionSpec NumberDecimalInfo::methods[2] = {
    MONGO_ATTACH_JS_FUNCTION(toString), JS_FS_END,
};

const char* const NumberDecimalInfo::className = "NumberDecimal";

namespace {
const char* const kValue = "value";
}  // namespace

Decimal128 NumberDecimalInfo::ToNumberDecimal(JSContext* cx, JS::HandleValue thisv) {
    JS::RootedObject obj(cx, thisv.toObjectOrNull());
    return ToNumberDecimal(cx, obj);
}

Decimal128 NumberDecimalInfo::ToNumberDecimal(JSContext* cx, JS::HandleObject thisv) {
    ObjectWrapper o(cx, thisv);

    return Decimal128(o.getNumberDecimal(kValue));
}

void NumberDecimalInfo::Functions::toString(JSContext* cx, JS::CallArgs args) {
    str::stream ss;

    Decimal128 val = NumberDecimalInfo::ToNumberDecimal(cx, args.thisv());

    ss << "NumberDecimal(\"" << val.toString() << "\")";

    ValueReader(cx, args.rval()).fromStringData(ss.operator std::string());
}

void NumberDecimalInfo::construct(JSContext* cx, JS::CallArgs args) {
    auto scope = getScope(cx);

    JS::RootedObject thisv(cx);

    scope->getNumberDecimalProto().newObject(&thisv);

    std::string decimalString = "0";

    if (args.length() == 0) {
        // Do nothing
    } else if (args.length() == 1) {
        decimalString = ValueWriter(cx, args.get(0)).toString();
    } else {
        uasserted(ErrorCodes::BadValue, "NumberDecimal takes 0 or 1 arguments");
    }

    ObjectWrapper o(cx, thisv);
    o.setString("value", decimalString);

    args.rval().setObjectOrNull(thisv);
}

}  // namespace mozjs
}  // namespace mongo
