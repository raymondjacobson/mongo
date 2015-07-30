db.doubleBank.drop();

var initialBalance = 20000000000;
var toDong = 21819.00;
var fromDong = 0.0000461;

db.doubleBank.insert({
	balance : initialBalance 
});

Math.seed = 6;
Math.seededRandom = function(max, min) {
    max = max || 1;
    min = min || 0;
 
    Math.seed = (Math.seed * 9301 + 49297) % 233280;
    var rnd = Math.seed / 233280;
 
    return min + rnd * (max - min);
}

var iterations = 500;
for (var i = 0; i < iterations; ++i) {
	var transactionAmount = Math.seededRandom()*10000000;
	var direction = (Math.seededRandom() > 0.5 ? 1 : -1);
	transactionAmount *= direction;

	db.doubleBank.update({}, {$mul : {balance : toDong} });
	db.doubleBank.update({}, {$inc : {balance : transactionAmount} });
	db.doubleBank.update({}, {$mul : {balance : fromDong} });
}