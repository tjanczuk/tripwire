// Time boxes execution of a piece of untrusted code to 2 seconds

var tripwire = require('../lib/tripwire.js');

var untrustedCode = function () {
	console.log('[Malcolm] Ha ha ha! I am taking over your event loop, Albert. Ha ha ha!');
	while(true);
};

process.on('uncaughtException', function (e) {
	// This code will execute so you can do some logging. 
	// Note that untrusted code should be prevented from registring to this event.
	console.log('[Albert] Sorry Malcolm, but you overstayed your welcome.');
	process.exit(1);
});

// Terminate the process if the even loop is not responding within 2-4 seconds
tripwire.resetTripwire(2000);

// Now execute some untusted code
untrustedCode();

// This line would normally clear the tripwire, but it will never be reached due to 
// the runaway code that proceeds it
tripwire.clearTripwire(); 
