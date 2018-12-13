function get_native_path() {
	var native_path = process.env.TRIPWIRE_NATIVE;

	if (!native_path) {
		let majorVersion = parseInt(process.versions.node.split('.')[0]);
		
		if (process.platform === 'win32' && majorVersion >= 8 && majorVersion <= 11)
			native_path = './native/win32/' + majorVersion + '/' + process.arch + '/tripwire';
		else
			native_path = require('path').join(__dirname, '../build/Release/tripwire');
	}

	return native_path;
}

module.exports = require(get_native_path());
