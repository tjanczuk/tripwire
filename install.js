var exec_process = require('child_process').exec("node-gyp configure build");
exec_process.stdout.pipe(process.stdout)
