#include <process.h>
#include <node.h>
#include <v8.h>

using namespace v8;

HANDLE scriptThread;
DWORD tripwireThreshold;
HANDLE tripwireThread;
HANDLE event;
Persistent<Value> context;
BOOL terminated;

void tripwireWorker(void* data)
{
	BOOL skipTimeCapture = FALSE;
	ULARGE_INTEGER su, sk, eu, ek;
	FILETIME tmp;

	// This thread monitors the elapsed CPU utilization time of the node.js thread and forces V8 to terminate
	// execution if it exceeds the preconfigured tripwireThreshold.

	while (1) 
	{
		// Unless the threshold validation logic requested to keep the current thread time utilization values,
		// capture the current user mode and kernel mode CPU utilization time of the thread on which node.js executes
		// application code. 

		if (skipTimeCapture) 
			skipTimeCapture = FALSE;
		else 
			GetThreadTimes(scriptThread, &tmp, &tmp, (LPFILETIME)&sk.u, (LPFILETIME)&su.u);

		// Wait on the auto reset event. The event will be signalled in one of two cases:
		// 1. When the timeout value equal to tripwireThreshold elapses, or
		// 2. When the event is explicitly signalled from resetTripwire.
		// A tripwireThreshold value of 0 indicates the tripwire mechanism is turned off, in which case
		// an inifite wait is initiated on the event (which will only be terminated with an explicit signal
		// during subsequent call to resetThreashold).

		if (WAIT_TIMEOUT == WaitForSingleObject(event, 0 == tripwireThreshold ? INFINITE : tripwireThreshold)) 
		{
			// If the wait result on the event is WAIT_TIMEOUT, it means neither clearThreshold or resetThreshold
			// were called in the tripwireThreshold period since the last call to resetThreshold. This indicates
			// a possibility that the node.js thread is blocked. 

			// If tripwireThreshold is 0 at this point, however, it means a call to clearTripwire was made 
			// since the last call to resetThreshold. In this case we just skip tripwire enforcement and 
			// proceed to wait for a subsequent event. 

			if (0 < tripwireThreshold) 
			{
				// Take a snapshot of the current kernel and user mode CPU utilization time of the node.js thread
				// to determine if the elapsed CPU utilization time exceeded the preconfigured tripwireThreshold. 
				// Despite the fact this code only ever executes after the auto reset event has already timeout out 
				// after the tripwireThreshold amount of time without hearing from the node.js thread, it need not 
				// necessarily mean that the node.js thread exceeded that execution time threshold. It might not
				// have been running at all in that period, subject to OS scheduling. 

				GetThreadTimes(scriptThread, &tmp, &tmp, (LPFILETIME)&ek.u, (LPFILETIME)&eu.u);
				ULONGLONG elapsed100Ns = ek.QuadPart - sk.QuadPart + eu.QuadPart - su.QuadPart;

				// Thread execution times are reported in 100ns units. Convert to milliseconds.

				DWORD elapsedMs = elapsed100Ns / 10000;
				
				// If the actual CPU execution time of the node.js thread exceeded the threshold, terminate
				// the V8 process. Otherwise wait again while maintaining the current snapshot of the initial
				// time utilization. This mechanism results in termination of a runaway thread some time in the
				// (tripwireThreshold, 2 * tripwireThreshold) range of CPU utilization.

				if (elapsedMs >= tripwireThreshold)
				{
					terminated = TRUE;
					V8::TerminateExecution();
				}
				else
				{
					skipTimeCapture = TRUE;
				}
			}
		}
	}
}

Handle<Value> clearTripwire(const Arguments& args) 
{
    HandleScope scope;

    // Seting tripwireThreshold to 0 indicates to the worker process that
    // there is no threshold to enforce. The worker process will make this determination
    // next time it is signalled, there is no need to force an extra context switch here
    // by explicit signalling. 

	tripwireThreshold = 0;
	terminated = FALSE;
	context.Dispose();
	context.Clear();

    return Undefined();
}

Handle<Value> resetTripwire(const Arguments& args)
{
    HandleScope scope;

	if (0 == args.Length() || !args[0]->IsUint32())
		return ThrowException(Exception::Error(String::New(
			"First agument must be an integer time threshold in milliseconds.")));

	if (0 == args[0]->ToUint32()->Value())
		return ThrowException(Exception::Error(String::New(
			"The time threshold for blocking operations must be greater than 0.")));

	clearTripwire(args);

	tripwireThreshold = args[0]->ToUint32()->Value();
	if (args.Length() > 1) 
	{
		context = Persistent<Value>::New(args[1]);
	}

    if (NULL == tripwireThread) 
    {
    	// This is the first call to resetTripwire. Perform lazy initialization.

    	// Create the auto reset event that will be used for signalling future changes
    	// of the tripwireThreshold value to the worker thread. 

    	if (NULL == (event = CreateEvent(NULL, FALSE, FALSE, NULL)))
    		return ThrowException(Exception::Error(String::New("Unable to create waitable event.")));

    	// Capture the current thread handle as the thread on which node.js executes user code. The
    	// worker process measures the CPU utilization of this thread to determine if the execution time
    	// threshold has been exceeded. 

    	if (!DuplicateHandle(
	    		GetCurrentProcess(), 
	    		GetCurrentThread(), 
	    		GetCurrentProcess(),
	    		&scriptThread,
	    		0,
	    		FALSE,
	    		DUPLICATE_SAME_ACCESS)) 
    	{
    		CloseHandle(event);
    		event = NULL;
       		return ThrowException(Exception::Error(String::New("Unable to duplicate handle of the script thread.")));
    	}

    	// Create the worker thread.

    	if (NULL == (tripwireThread = (HANDLE)_beginthread(tripwireWorker, 4096, NULL)))
    	{
    		CloseHandle(event);
    		event = NULL;
      		CloseHandle(scriptThread);
      		scriptThread = 0;
    		return ThrowException(Exception::Error(String::New("Unable to initialize a tripwire thread.")));
    	}
    }
    else 
    {
    	// Signal the already existing worker process using the auto reset event. 
    	// This will cause the worker process to 
    	// reset the elapsed time timer and pick up the new tripwireThreshold value.

    	SetEvent(event);
    }

    return Undefined();
}

Handle<Value> getContext(const Arguments& args) 
{
    HandleScope scope;

    // If the script had been terminated by tripwire, returns the context passed to resetTripwire;
    // otherwise undefined. This can be used from within the uncaughtException handler to determine
    // whether the exception condition was caused by script termination.

    if (terminated)
    	return context;
    else
    	return Undefined();
}

void init(Handle<Object> target) 
{
  	tripwireThread = scriptThread = event = NULL;
  	terminated = FALSE;
    NODE_SET_METHOD(target, "resetTripwire", resetTripwire);
    NODE_SET_METHOD(target, "clearTripwire", clearTripwire);
    NODE_SET_METHOD(target, "getContext", getContext);
}

NODE_MODULE(tripwire, init);