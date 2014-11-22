if (process.platform !== 'win32') {
    require('child_process').spawn('node-gyp', ['rebuild'], {
        cwd: __dirname,
        stdio: 'inherit'
    }).on('close', process.exit).on('error', console.log);
}