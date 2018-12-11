var native_path = process.env.TRIPWIRE_NATIVE;
if (!native_path) {
    if (process.platform === 'win32') {
    	if (process.versions) {
			var vers = process.versions.node.split('.'),
				majorVersion = parseInt(vers.shift()),
				minorVersion = parseInt(vers.shift()),
				patchVersion = vers.length ? parseInt(vers.shift()) : 0;

			if (majorVersion > 3)
				native_path = './native/win32/4.0/' + process.arch + '/tripwire';
			else if (majorVersion === 0) {
				if (minorVersion > 11)
					native_path = './native/win32/0.12/' + process.arch + '/tripwire';
				else
					native_path = './native/win32/0.10/' + process.arch + '/tripwire';
			}
			else
				native_path = './native/win32/0.12/' + process.arch + '/tripwire';
		}
		else {
			if (process.version.match(/^v4\./))
				native_path = './native/win32/4.0/' + process.arch + '/tripwire';
			else if (process.version.match(/^v0\.12/))
				native_path = './native/win32/0.12/' + process.arch + '/tripwire';
			else
				native_path = './native/win32/0.10/' + process.arch + '/tripwire';
}
    }
    else
        native_path = require('path').join(__dirname, '../build/Release/tripwire');
}
module.exports = require(native_path);
