Tripwire
========

Tripwire allows node.js applications to termiante execution of scripts that block the node.js event loop. For example, you can break out from infinite loops like `while(true)`. This functionality is useful if you are executing untrusted code within your node.js process. 

Tripwire contains a native extension of node.js and currently only supports Windows. I do take contributions. 

Install with:

```
npm install tripwire
```

Then in your application, you can put a limit on the total amout of CPU time (kernel and user mode combined) the event loop is blocked before the execution of the script is terminated:

```javascript
var tripwire = require('tripwire');

process.on('uncaughtException', function (e) {
  console.log('The event loop was blocked for longer than 2000 milliseconds');
  process.exit(1);
});

// set the limit of execution time to 2000 milliseconds
tripwire.resetTripwire(2000);

// execute code that will block the event loop for longer
while(true);

// clear the tripwire (in this case this code is never reached)
tripwire.clearTripwire();
```

For more samples, see [here](https://github.com/tjanczuk/tripwire/tree/master/samples).

#### Running tests

There are a few mocha tests included that you can run with

```
mocha -R List
```

#### Building

The native component is included in the repository and not built during `npm install tripwire`.

You can rebuild the native component using [node-gyp](https://github.com/TooTallNate/node-gyp/). Currently the native component can be compiled on Windows only (I do take contributions).

```
node-gyp configure build
copy build\Release\tripwire.node lib\native\windows\x86\
```