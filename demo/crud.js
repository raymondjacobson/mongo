// Example 1 - Exact addition/accumulation

db.numbers.drop();
db.numbers.insert({a: 0.1, b: NumberDecimal("0.1")});

db.numbers.update({}, {$inc: {a: 0.1}}, {multi: 1});
db.numbers.update({}, {$inc: {a: 0.1}}, {multi: 1});
db.numbers.update({}, {$inc: {b: NumberDecimal("0.1")}}, {multi: 1});
db.numbers.update({}, {$inc: {b: NumberDecimal("0.1")}}, {multi: 1});

db.numbers.find();
db.numbers.find({a: 0.3});
db.numbers.find({b: NumberDecimal("0.3")});

// Example 2 -- We can store trailing zeros

db.numbers.drop();
db.numbers.insert({a: NumberDecimal("5.0")});
db.numbers.insert({a: NumberDecimal("5.00")});

db.numbers.find({a: NumberDecimal("5")});

// Example 3 -- We can query with other types and compare well

db.numbers.find({a: 5});
db.numbers.find({a: 5.0});
db.numbers.find({a: NumberLong("5")});

// Example 3 -- Cross type equality

db.numbers.insert({a: 5});
db.numbers.insert({a: 5.0});
db.numbers.insert({a: NumberLong("5")});
db.numbers.insert({a: NumberDecimal("6")});

db.numbers.find({a: 5});


db.numbers.find({a: {$gt: 5}});

// Example 4 -- Multiplication

db.numbers.drop();
db.numbers.insert({a: NumberDecimal("10201.01")});
db.numbers.find();

// Example 5 -- Mixed Operations

db.numbers.update({}, {$mul: {a: NumberLong("1000000000000")}});


db.numbers.drop();
db.numbers.insert({a: NumberDecimal("10201.01")});
db.numbers.find();

db.numbers.update({}, {$mul: {a: NumberDecimal("1E12")}});
