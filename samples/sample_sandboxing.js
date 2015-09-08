// Time boxes execution of a piece of untrusted code to 2 seconds

var tripwire = require('../lib/tripwire.js');

var untrustedCode = function () {
	console.log('[Malcolm] Ha ha ha! I am taking over your event loop, Albert. Ha ha ha!');
	while(true);
};

process.on('uncaughtException', function (e) {
	// This code will execute so you can do some logging. 
	// Note that untrusted code should be prevented from registring to this event.

	// If tripwire caused this exception, tripwire.getContext() will return the 
	// context object passed to tripwire.resetTripwire.

	var context = tripwire.getContext();
	console.log('IN U', context);
	if (context)
		console.log('[Albert] Sorry ' + context.codeOwner + ', but you overstayed your welcome.');

	process.exit(1);
});

// Terminate the process if the even loop is not responding within 2-4 seconds
var context = { codeOwner: 'Malcolm' };
tripwire.resetTripwire(2000, context);

// Now execute some untusted code
untrustedCode();

// This line would normally clear the tripwire, but it will never be reached due to 
// the runaway code that proceeds it
tripwire.clearTripwire(); 
