function get_native_path() {
	var native_path = process.env.TRIPWIRE_NATIVE;
	if (!native_path) {
		native_path = require('path').join(__dirname, '../build/Release/tripwire');
	}
	
	return native_path;
}

module.exports = require(native_path);
