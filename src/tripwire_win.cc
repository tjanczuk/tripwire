#include <process.h>
#include <node.h>
#include <v8.h>

using namespace v8;

HANDLE scriptThread;
HANDLE tripwireThread;
HANDLE event;

extern unsigned int tripwireThreshold;
extern int terminated;

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
			// If the wait result on the event is WAIT_TIMEOUT, it means resetThreshold
			// was called in the tripwireThreshold period since the last call to resetThreshold. This indicates
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
					terminated = 1;
					V8::TerminateExecution();
				}
				else
				{
					skipTimeCapture = 1;
				}
			}
		}
	}
}

Handle<Value> resetTripwireCore()
{
    HandleScope scope;

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
    	// Signal the already existing worker thread using the auto reset event. 
    	// This will cause the worker thread to 
    	// reset the elapsed time timer and pick up the new tripwireThreshold value.

    	SetEvent(event);
    }

    return Undefined();
}

void initCore() 
{
	tripwireThread = scriptThread = event = NULL;
}
