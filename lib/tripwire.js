var native_path = process.env.TRIPWIRE_NATIVE || ('./native/' + process.platform + '/'  + process.arch + '/tripwire');
if (process.platform === 'win32' || process.platform === 'darwin' || process.platform === 'linux')
	module.exports = require(native_path)
else
	throw new Error('The tripwire module is currently only suppored on Windows, Mac, and Linux. I do take contributions. '
		+ 'https://github.com/tjanczuk/tripwire');
