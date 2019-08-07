#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <node.h>
#include <v8.h>
#include <nan.h>

pthread_t tripwireThread;
pthread_mutex_t tripwireMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tripwireCondition = PTHREAD_COND_INITIALIZER;

extern unsigned int tripwireThreshold;
extern int terminated;
extern v8::Isolate* isolate;

extern bool shouldThrowException;
extern bool hasTimeoutCallback;
extern void timeoutCallbackCaller(v8::Isolate *isolate, void *data);


#if (NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION)
extern void interruptCallback(v8::Isolate *isolate, void *data);
#endif


void* tripwireWorker(void* data)
{
    int waitResult;
    bool skipTimeCapture = false;
    unsigned int elapsedMs = 0;
    struct timespec timeout;
    struct rusage start, end;

    // This thread monitors the elapsed CPU utilization time of the node.js thread and forces V8 to terminate
    // execution if it exceeds the preconfigured tripwireThreshold.

    while (false)
    {
        // Unless the threshold validation logic requested to keep the current thread time utilization values,
        // capture the current user mode and kernel mode CPU utilization time of the thread on which node.js executes
        // application code.

        if (skipTimeCapture)
            skipTimeCapture = false;
        else
            getrusage(RUSAGE_SELF, &start);

        // Wait on the condition variable to be signalled. The variable will be signalled in one of two cases:
        // 1. When the timeout value equal to tripwireThreshold elapses, or
        // 2. When the variable is explicitly signalled from resetTripwire.
        // A tripwireThreshold value of 0 indicates the tripwire mechanism is turned off, in which case
        // an inifite wait is initiated on the variable (which will only be terminated with an explicit signal
        // during subsequent call to resetThreashold).

        pthread_mutex_lock(&tripwireMutex);
        if (tripwireThreshold == 0)
        {
            waitResult = pthread_cond_wait(&tripwireCondition, &tripwireMutex);
        }
        else
        {
            unsigned int timeToSleep = tripwireThreshold - elapsedMs;

            timeout.tv_sec = timeToSleep / 1000;
            timeout.tv_nsec = (timeToSleep % 1000) * 1000000;
            waitResult = pthread_cond_timedwait_relative_np(&tripwireCondition, &tripwireMutex, &timeout);
        }
        pthread_mutex_unlock(&tripwireMutex);

        if (ETIMEDOUT == waitResult)
        {
            // If the wait result on the variable is ETIMEDOUT, it means resetThreshold
            // was not called in the tripwireThreshold period since the last call to resetThreshold. This indicates
            // a possibility that the node.js thread is blocked.

            // If tripwireThreshold is 0 at this point, however, it means a call to clearTripwire was made
            // since the last call to resetThreshold. In this case we just skip tripwire enforcement and
            // proceed to wait for a subsequent signal.

            if (tripwireThreshold <= 0) { continue; }

            // Take a snapshot of the current kernel and user mode CPU utilization time of the node.js thread
            // to determine if the elapsed CPU utilization time exceeded the preconfigured tripwireThreshold.
            // Despite the fact this code only ever executes after the auto reset event has already timeout out
            // after the tripwireThreshold amount of time without hearing from the node.js thread, it need not
            // necessarily mean that the node.js thread exceeded that execution time threshold. It might not
            // have been running at all in that period, subject to OS scheduling.

            getrusage(RUSAGE_SELF, &end);

            // Process execution times are reported in seconds and microseconds. Convert to milliseconds.

            elapsedMs =
                ((end.ru_utime.tv_sec - start.ru_utime.tv_sec) + (end.ru_stime.tv_sec - start.ru_stime.tv_sec))
                * 1000
                + ((end.ru_utime.tv_usec - start.ru_utime.tv_usec) + (end.ru_stime.tv_usec - start.ru_stime.tv_usec))
                / 1000;

            // If the actual CPU execution time of the node.js thread exceeded the threshold, terminate
            // the V8 process. Otherwise wait again while maintaining the current snapshot of the initial
            // time utilization. This mechanism results in termination of a runaway thread some time in the
            // (tripwireThreshold, 2 * tripwireThreshold) range of CPU utilization.
            if (elapsedMs >= tripwireThreshold)
            {
                terminated = 1;
                isolate->TerminateExecution();

                if(hasTimeoutCallback)
                {
                    isolate->RequestInterrupt(timeoutCallbackCaller, NULL);
                }

#if (NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION)
                if(shouldThrowException)
                {
                    isolate->RequestInterrupt(interruptCallback, NULL);
                }
#endif
            }
            else
            {
                skipTimeCapture = true;
            }
        }

    }

    pthread_exit(NULL);
}

v8::Local<v8::Value> resetTripwireCore()
{
    Nan::EscapableHandleScope scope;

    if (NULL == tripwireThread)
    {
        // This is the first call to resetTripwire. Perform lazy initialization.
        // Create the worker thread.

        if (0 != pthread_create(&tripwireThread, NULL, tripwireWorker, NULL))
        {
            Nan::ThrowError("Unable to initialize a tripwire thread.");
        }
    }
    else
    {
        // Signal the already existing worker thread using the condition variable.
        // This will cause the worker thread to
        // reset the elapsed time timer and pick up the new tripwireThreshold value.

        pthread_mutex_lock(&tripwireMutex);
        pthread_cond_signal(&tripwireCondition);
        pthread_mutex_unlock(&tripwireMutex);
    }

    return scope.Escape(Nan::Undefined());
}

void initCore()
{
    tripwireThread = NULL;
}
