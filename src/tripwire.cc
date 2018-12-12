#include <node.h>
#include <v8.h>
#include <nan.h>

extern v8::Local<v8::Value> resetTripwireCore();
extern void initCore();

unsigned int tripwireThreshold;
Nan::Persistent<v8::Value> context;
int terminated;
v8::Isolate* isolate;

#if (NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION)
void interruptCallback(v8::Isolate* isolate, void* data) 
{
    Nan::RunScript(Nan::CompileScript(Nan::New<v8::String>(
        "process.nextTick(function () { throw new Error('Blocked event loop'); });"
    ).ToLocalChecked()).ToLocalChecked());
}
#endif

NAN_METHOD(clearTripwire)
{
    // Seting tripwireThreshold to 0 indicates to the worker process that
    // there is no threshold to enforce. The worker process will make this determination
    // next time it is signalled, there is no need to force an extra context switch here
    // by explicit signalling. 

	tripwireThreshold = 0;
	terminated = 0;
    context.Reset();
    info.GetReturnValue().SetUndefined();
}

NAN_METHOD(resetTripwire)
{
    Nan::EscapableHandleScope scope;
	if (0 == info.Length()  || Nan::To<v8::Uint32>(info[0]).IsEmpty()) {
        Nan::ThrowError("First agument must be an integer time threshold in milliseconds.");
        info.GetReturnValue().SetUndefined();
    }
    else if (0 == Nan::To<v8::Uint32>(info[0]).ToLocalChecked()->Value()) {
        Nan::ThrowError("The time threshold for blocking operations must be greater than 0.");
        info.GetReturnValue().SetUndefined();
    }
    else {
    	clearTripwire(info);

    	tripwireThreshold = Nan::To<v8::Uint32>(info[0]).ToLocalChecked()->Value();
        isolate = info.GetIsolate();
    	if (info.Length() > 1) 
    	{
            context.Reset(info[1]);
    	}

        info.GetReturnValue().Set(resetTripwireCore());
    }
}

NAN_METHOD(getContext)
{
    Nan::EscapableHandleScope scope;

    // If the script had been terminated by tripwire, returns the context passed to resetTripwire;
    // otherwise undefined. This can be used from within the uncaughtException handler to determine
    // whether the exception condition was caused by script termination.

    if (terminated)
        info.GetReturnValue().Set(Nan::New(context));
    else
    	info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(init) 
{
    initCore();
    isolate = v8::Isolate::GetCurrent();
    terminated = 0;
    Nan::Set(target,
        Nan::New<v8::String>("resetTripwire").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(resetTripwire)->GetFunction());
    Nan::Set(target,
        Nan::New<v8::String>("clearTripwire").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(clearTripwire)->GetFunction());
    Nan::Set(target,
        Nan::New<v8::String>("getContext").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(getContext)->GetFunction());
}

NODE_MODULE(tripwire, init);
