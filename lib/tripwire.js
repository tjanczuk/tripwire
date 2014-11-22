var native_path = process.env.TRIPWIRE_NATIVE;
if (!native_path) {
    if (process.platform === 'win32')
        native_path = './native/win32/' + process.arch + '/tripwire';
    else
        native_path = require('path').join(__dirname, '../build/Release/tripwire');
}
module.exports = require(native_path);
