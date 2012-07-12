var tripwire = require('../lib/tripwire.js')
	, assert = require('assert');

var validator;

process.on('uncaughtException', function (e) {
	if (validator) {
		var tmp = validator;
		validator = undefined;
		tmp();
	}
	else {
		console.log('Unexpected uncaughtException event.');
		process.exit(1);
	}
});

describe('tripwire', function () {

	it('terminates infinite loop', function (done) {
		validator = done;
		tripwire.resetTripwire(100);
		while(true);
	});

	it('can be cleared', function (done) {
		validator = function () {
			assert.ok(false, 'tripire unexpectedly went off');
		};

		tripwire.resetTripwire(50);
		tripwire.clearTripwire();
		setTimeout(done, 100);
	});

	it('can be reset', function (done) {
		validator = done;
		tripwire.resetTripwire(50);
		tripwire.resetTripwire(100);
		while(true);
	});

});