// Breaks out from the infinite loop after 2 seconds

var tripwire = require('../lib/tripwire.js');

process.on('uncaughtException', function (e) {
	// This code will execute so you can do some logging. 
	// Note that untrusted code should be prevented from registring to this event.
	console.log('In uncaughtException event.');
	process.exit(1);
});

// Terminate the process if the even loop is not responding within 2-4 seconds
tripwire.resetTripwire(2000);

try {
	// Without tripwire, the following line of code would block the node.js
	// event loop forever.
	while(true);
}
catch (e) {
	// This code will never execute. The only place this exception can be called is
	// in uncaughtException handler.
	console.log('Caught exception.');
}

// This line would normally clear the tripwire, but it will never be reached due to 
// the infinite loop that preceeds it.
tripwire.clearTripwire(); 
