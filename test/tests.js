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
			validator = undefined;
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

	it('propagates context after termination', function (done) {
		var context = { foo: "bar" };
		validator = function () {
			validator = undefined;
			assert.ok(context === tripwire.getContext(), 'the context matches');
			done();
		};
		tripwire.resetTripwire(100, context);
		while(true);
	});

	it('does not propagate context if cleared', function () {
		tripwire.resetTripwire(50, {});
		tripwire.clearTripwire();
		assert.ok(undefined === tripwire.getContext(), 'no context returned from tripwire');
	});

	it('requires threshold to be positive integer', function (done) {
		assert.throws(
			function () { tripwire.resetTripwire(); },
			/First agument must be an integer time threshold in milliseconds/
		);
	
		assert.throws(
			function () { tripwire.resetTripwire({}); },
			/First agument must be an integer time threshold in milliseconds/
		);

		assert.throws(
			function () { tripwire.resetTripwire(0); },
			/The time threshold for blocking operations must be greater than 0/
		);

		assert.throws(
			function () { tripwire.resetTripwire(-100); },
			/First agument must be an integer time threshold in milliseconds/
		);

		done();
	});

});