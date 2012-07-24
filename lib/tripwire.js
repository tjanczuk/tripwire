if (process.platform === 'win32')
	module.exports = require('./native/windows/x86/tripwire');
else if (process.platform === 'darwin')
	module.exports = require('./native/darwin/x86/tripwire')
else
	throw new Error('The tripwire module is currently only suppored on Windows and Mac. I do take contributions. '
		+ 'https://github.com/tjanczuk/tripwire');
