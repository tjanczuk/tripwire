#include <node.h>
#include <v8.h>
#include <nan.h>

extern v8::Local<v8::Value> resetTripwireCore();
extern void initCore();

unsigned int tripwireThreshold;
Nan::Persistent<v8::Value> context;
int terminated;
v8::Isolate* isolate;
bool shouldThrowException = true;
bool useRealTime = false;

bool hasTimeoutCallback = false;
v8::Local<v8::Value> argv[] {Nan::Null()};
Nan::Callback timeoutCallback;

#if (NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION)
void interruptCallback(v8::Isolate* isolate, void* data) 
{
    Nan::RunScript(Nan::CompileScript(Nan::New<v8::String>(
        "process.nextTick(function () { throw new Error('Blocked event loop'); });"
        ).ToLocalChecked()
    ).ToLocalChecked());
}
#endif

void timeoutCallbackCaller(v8::Isolate* isolate, void* data)
{
   Nan::Call(timeoutCallback, 1, argv);
}

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

NAN_METHOD(setShouldThrowException)
{
    Nan::EscapableHandleScope scope;

    if(info.Length() == 0 || Nan::To<v8::Boolean>(info[0]).IsEmpty()) {
        Nan::ThrowError("First agument must be a boolean.");
        info.GetReturnValue().SetUndefined();
    }
    else {
        shouldThrowException = Nan::To<v8::Boolean>(info[0]).ToLocalChecked()->IsTrue();
    }
}

NAN_METHOD(setUseRealTime)
{
    Nan::EscapableHandleScope scope;

    if(info.Length() == 0 || Nan::To<v8::Boolean>(info[0]).IsEmpty()) {
        Nan::ThrowError("First agument must be a boolean.");
        info.GetReturnValue().SetUndefined();
    }
    else {
        useRealTime = Nan::To<v8::Boolean>(info[0]).ToLocalChecked()->IsTrue();
    }
}

NAN_METHOD(setTimeoutCallback)
{
    Nan::EscapableHandleScope scope;

     if(info.Length() == 0 || Nan::To<v8::Function>(info[0]).IsEmpty()) {
        Nan::ThrowError("First agument must be a function.");
        info.GetReturnValue().SetUndefined();
    }
    else {
        hasTimeoutCallback = true;
        const v8::Local<v8::Function> func = Nan::To<v8::Function>(info[0]).ToLocalChecked();
        timeoutCallback.Reset(func);
    }
}

void cleanUpFunction(void*)
{
    timeoutCallback.Reset();
}

NAN_MODULE_INIT(init) 
{
    initCore();
    isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    terminated = 0;
    Nan::Set(target,
        Nan::New<v8::String>("resetTripwire").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(resetTripwire)->GetFunction(context).ToLocalChecked());
    Nan::Set(target,
        Nan::New<v8::String>("clearTripwire").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(clearTripwire)->GetFunction(context).ToLocalChecked());
    Nan::Set(target,
        Nan::New<v8::String>("getContext").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(getContext)->GetFunction(context).ToLocalChecked());

    Nan::Set(target,
        Nan::New<v8::String>("setShouldThrowException").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(setShouldThrowException)->GetFunction(context).ToLocalChecked());
    Nan::Set(target,
        Nan::New<v8::String>("setUseRealTime").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(setUseRealTime)->GetFunction(context).ToLocalChecked());
    Nan::Set(target,
        Nan::New<v8::String>("setTimeoutCallback").ToLocalChecked(),
        Nan::New<v8::FunctionTemplate>(setTimeoutCallback)->GetFunction(context).ToLocalChecked());

    node::AtExit(cleanUpFunction, NULL);
}

NODE_MODULE(tripwire, init);
