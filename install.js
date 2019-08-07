let majorVersion = parseInt(process.versions.node.split('.')[0]);

if (!(process.platform === 'win32' && majorVersion >= 8 && majorVersion <= 11)) {
	let exec_process = require('child_process').exec("node-gyp configure build");
	exec_process.stdout.pipe(process.stdout);
}
