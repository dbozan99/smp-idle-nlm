#include <signal.h>
#include <nks/thread.h> 
#include <nks/synch.h>

volatile int boolRunning = TRUE;
volatile int intActiveThreads = 0;

NXMutex_t *mutThreadCount = NULL;
NX_LOCK_INFO_ALLOC(g_mutexInfo, "SMP-IDLE Thread Count Lock", 0);

void idle_worker(void *arg)
{
    NXCpuId_t target_cpu = (NXCpuId_t)(long)arg; 

    NXThreadBind(target_cpu);
    NXThreadDelay(50); //Make sure that the next time we wake up we'll actually be on the target_cpu

    while (boolRunning) {
        _asm {
            sti
            hlt
        }
        // Yielding to the scheduler is still required, otherwise we'll hog the cpu
        // We can't use NXThreadYield though, as that wouldn't give the scheduler a chance to run other threads.
        NXThreadDelay(1);
    }

    // Thread is exiting. Decrement active thread count and exit.
    NXLock(mutThreadCount);
    intActiveThreads--;
    NXUnlock(mutThreadCount);

    NXThreadExit(NULL);
}

void sigterm_handler(int sig)
{
    (void)sig; // Suppress unused parameter warning
    boolRunning = FALSE;
    printf("SMP-IDLE: [INFO] Stopping all idle-worker threads\n");
}

int main(int argc, char **argv)
{
    int cpuCount, i;
    mutThreadCount = NXMutexAlloc(0, 0, &g_mutexInfo);
    
    // Register SIGTERM to catch the "UNLOAD SMP-IDLE" server command
    signal(SIGTERM, sigterm_handler);
    
    cpuCount = NXGetCpuCount();
    printf("\nSMP-IDLE: [INFO] CPUs detected = %d\n", cpuCount);

    boolRunning = TRUE; 
    intActiveThreads = 0;
    for (i = 0; i < cpuCount; i++) {
        NXContext_t ctx;
        NXThreadId_t tid;
        // Create a thread for each CPU
        ctx = NXContextAlloc(idle_worker, (void *)(long)i, NX_PRIO_HIGH, 16384, NX_CTX_NORMAL, NULL);
        if (ctx) {
            if (NXThreadCreate(ctx, NX_THR_BIND_CONTEXT, &tid) == 0) {
                NXLock(mutThreadCount);
                intActiveThreads++;
                NXUnlock(mutThreadCount);
            }
            else {
                printf("SMP-IDLE: [ERROR] Couldn't create thread on CPU%d \n", i);
                NXContextFree(ctx); // Prevent memory leak if creation fails
            }
        }
    }
    
    printf("SMP-IDLE: [INFO] Idle-worker threads started = %d \n", intActiveThreads);

    // Instead of ExitThread(TSR_THREAD), LibC NLMs stay resident by keeping main() alive.
    // Yield to the CPU time efficiently back to the OS.
    while (boolRunning)  {
        NXThreadDelay(100); 
    }
    
    // Unloading. Wait for worker threads exit.
    while (intActiveThreads > 0) {
        NXThreadDelay(10); 
    }

    // Final cleanup
    if (mutThreadCount) {
        NXMutexFree(mutThreadCount);
    }

    return 0;
}
