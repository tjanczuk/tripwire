#include <node.h>
#include <v8.h>
#include <nan.h>

using namespace v8;

extern Handle<Value> resetTripwireCore();
extern void initCore();

unsigned int tripwireThreshold;
Persistent<Value> context;
int terminated;
v8::Isolate* isolate;

#if (NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION)
void interruptCallback(Isolate* isolate, void* data) 
{
    NanThrowError(NanNew<v8::String>("Blocked event loop."));
}
#endif

NAN_METHOD(clearTripwire)
{
    NanEscapableScope();

    // Seting tripwireThreshold to 0 indicates to the worker process that
    // there is no threshold to enforce. The worker process will make this determination
    // next time it is signalled, there is no need to force an extra context switch here
    // by explicit signalling. 

	tripwireThreshold = 0;
	terminated = 0;
    NanDisposePersistent(context);
    NanReturnValue(NanUndefined());
}

NAN_METHOD(resetTripwire)
{
    NanEscapableScope();
	if (0 == args.Length() || !args[0]->IsUint32()) {
        NanThrowError(NanNew<v8::String>(
            "First agument must be an integer time threshold in milliseconds."));
        NanReturnUndefined();
    }
    else if (0 == args[0]->ToUint32()->Value()) {
        NanThrowError(NanNew<v8::String>(
            "The time threshold for blocking operations must be greater than 0."));
        NanReturnUndefined();
    }
    else {
    	clearTripwire(args);

    	tripwireThreshold = args[0]->ToUint32()->Value();
        isolate = args.GetIsolate();
    	if (args.Length() > 1) 
    	{
            NanAssignPersistent(context, args[1]);
    	}

        NanReturnValue(resetTripwireCore());
    }
}

NAN_METHOD(getContext)
{
    NanEscapableScope();

    // If the script had been terminated by tripwire, returns the context passed to resetTripwire;
    // otherwise undefined. This can be used from within the uncaughtException handler to determine
    // whether the exception condition was caused by script termination.

    if (terminated)
    	NanReturnValue(NanNew(context));
    else
    	NanReturnValue(NanUndefined());
}

void init(Handle<Object> target) 
{
	initCore();
    isolate = v8::Isolate::GetCurrent();
  	terminated = 0;
    NODE_SET_METHOD(target, "resetTripwire", resetTripwire);
    NODE_SET_METHOD(target, "clearTripwire", clearTripwire);
    NODE_SET_METHOD(target, "getContext", getContext);
}

NODE_MODULE(tripwire, init);
