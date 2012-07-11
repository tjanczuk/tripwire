if (process.env.OS && process.env.OS.match(/windows/i))
	module.exports = require('./native/windows/x86/tripwire');
else
	throw new Error('The tripwire module is currently only suppored on Windows. I do take contributions. '
		+ 'https://github.com/tjanczuk/tripwire');
