Tripwire
========

Tripwire allows node.js applications to termiante execution of scripts that block the node.js event loop. For example, you can break out from infinite loops like `while(true)`. This functionality is useful if you are executing untrusted code within your node.js process. 

Tripwire contains a native extension of node.js and currently supports Windows, Mac, and Linux. I do take contributions. 

Install with:

Make sure you've installed all the [necessary build tools](https://github.com/TooTallNate/node-gyp#installation) for your platform, then invoke:

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

When the event loop is blocked for longer than the time specified in the call to `resetTripwire`, tripwire will terminate execution of the script. Node.js will subsequently execute the `uncaughtException` handler if one is registered. The exception passed to `uncaughtException` handler will be `null` in that case. In order to determine whether the exception was indeed caused by tripwire, an optional context can be established during a call to `resetTripwire` and retrtieved with a call to `getContext`. The `getContext` will return `undefined` if the tripwire had not been triggered. 

```javascript
var tripwire = require('tripwire');

process.on('uncaughtException', function (e) {
  if (undefined === tripwire.getContext())
    console.log('The exception was not caused by tripwire.');
  else
    console.log('The event loop was blocked for longer than 2000 milliseconds');
  process.exit(1);
});

// set the limit of execution time to 2000 milliseconds
var context = { someData: "foobar" };
tripwire.resetTripwire(2000, context);
```

For more samples, see [here](https://github.com/tjanczuk/tripwire/tree/master/samples).

#### Running tests

There are a few mocha tests included that you can run with

```
mocha -R list
```

#### Building

On OSX and Linux, the native component is built at installation time.

On Windows, the native component is included in the repository and not built during `npm install tripwire`.

You can rebuild the native component using [node-gyp](https://github.com/TooTallNate/node-gyp/). Currently the native component can be compiled on Windows, Mac, and Linux (I do take contributions).

On Windows:

```
node-gyp configure build
copy build\Release\tripwire.node lib\native\win32\4.0\ia32\
```
