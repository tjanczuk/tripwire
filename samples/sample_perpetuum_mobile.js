// Sets up a pepetual detection of blocked event loop

var tripwire = require('../lib/tripwire.js');

var untrustedCode = function (behaveEvil) {
	if (behaveEvil) {
		console.log('[Malcolm] Ha ha ha! I am taking over your event loop, Albert. Ha ha ha!');
		while(true);
	}
};

process.on('uncaughtException', function (e) {
	// This code will execute so you can do some logging. 
	// Note that untrusted code should be prevented from registring to this event.
	console.log('[Albert] Sorry Malcolm, but you overstayed your welcome.');
	process.exit(1);
});

// Perpetually reset the tripwire every 1 second using setInterval.
// This will terminate the process if the event loop is stopped for longer than 2 seconds
// because the setInterval callbacks will no longer be executing. 
tripwire.resetTripwire(2000);
setInterval(function () { 
	resetTripwire.resetTripwire(2000);
}, 1000);

// Now go execute any amount of untusted callbacks; one of them is evil
for (var i = 0; i < 1000; i++)
	untrustedCode(i === 666);