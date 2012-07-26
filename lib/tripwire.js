if (process.platform === 'win32' || process.platform === 'darwin' || process.platform === 'linux')
	module.exports = require('./native/' + process.platform + '/x86/tripwire')
else
	throw new Error('The tripwire module is currently only suppored on Windows, Mac, and Linux. I do take contributions. '
		+ 'https://github.com/tjanczuk/tripwire');
