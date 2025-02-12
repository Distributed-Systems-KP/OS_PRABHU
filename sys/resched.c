/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <q.h>
#include <lab1.h>

unsigned long currSP;  /* REAL sp of current process */
extern int ctxsw(int, int, int, int);

/*------------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:  Upon entry, currpid gives current process id.
 *         Proctab[currpid].pstate gives correct NEXT state for
 *         current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched() {
    register struct pentry *optr;  /* pointer to old process entry */
    register struct pentry *nptr;  /* pointer to new process entry */
    int schedclass = getschedclass();
    int current_ID;

    optr = &proctab[currpid];

    switch (schedclass) {
        case AGESCHED: {
            for (current_ID = q[rdyhead].qnext; current_ID != rdytail; current_ID = q[current_ID].qnext) {
                if (current_ID == NULLPROC) {
                    continue;
                }
                struct pentry *process_pointer = &proctab[current_ID];
                if (process_pointer->pprio <= __INT_MAX__ - 2) {
                    process_pointer->pprio += 2;
                    q[current_ID].qkey += 2;
                } else {
                    process_pointer->pprio = __INT_MAX__;
                    q[current_ID].qkey = __INT_MAX__;
                }
            }
            break;
        }

        case LINUXSCHED: {
            int pid;
            int runnable_in_epoch = 0;  // Flag to check if there is a process yet to finish

            // STEP 1: Check if any process in the ready queue belongs to the current epoch
            for (pid = q[rdyhead].qnext; pid != rdytail; pid = q[pid].qnext) {
                struct pentry *p = &proctab[pid];
                if (pid == NULLPROC) {
                    continue;
                }
                if (p->present_in_epoch && p->epoch == current_epoch && p->counter > 0) {
                    runnable_in_epoch = 1;
                    break;
                }
            }

            // Check the currently running process
            if (preempt <= 0) {
                optr->counter = 0;
            } else {
                optr->counter = preempt;
            }

            if (optr->counter <= 0) {
                optr->present_in_epoch = 0;
            }

            if (optr->pstate == PRCURR && optr->present_in_epoch && optr->epoch == current_epoch && optr->counter > 0) {
                runnable_in_epoch = 1;
            }

            // STEP 2: Increment the epoch if no process is runnable
            if (!runnable_in_epoch) {

                current_epoch++;
                for (pid = 1; pid < NPROC; pid++) {
                    struct pentry *p = &proctab[pid];
                    if (p->pstate == PRFREE) {
                        continue;
                    }
                    p->quantum = (p->counter > 0) ? (p->counter / 2) + p->pprio : p->pprio;
                    if (p->pstate == PRREADY || p->pstate == PRCURR) {
                        p->counter = p->quantum;
                        p->present_in_epoch = 1;
                        p->epoch = current_epoch;
                    } else {
                        p->present_in_epoch = 0;
                    }
                
                }
                // kprintf("//A new era begins\\\\");
            }

            // STEP 3: Calculate the goodness value of all active processes
            int max_goodness = 0;
            int chosen_pid = NULLPROC;

            if (optr->pstate == PRCURR && optr->present_in_epoch && optr->epoch == current_epoch && optr->counter > 0) {
                max_goodness = optr->counter + optr->pprio;
                chosen_pid = currpid;
            }

            for (pid = q[rdyhead].qnext; pid != rdytail; pid = q[pid].qnext) {
                struct pentry *p = &proctab[pid];
                if (p->present_in_epoch && p->epoch == current_epoch && p->counter > 0) {
                    int goodness = p->counter + p->pprio;
                    if (goodness >= max_goodness) {
                        max_goodness = goodness;
                        chosen_pid = pid;
                    }
                }
            }

            // STEP 4: Handle various cases when different processes are chosen from the ready queue
            if (chosen_pid == NULLPROC) {
                proctab[NULLPROC].present_in_epoch = 1;
                proctab[NULLPROC].epoch = current_epoch;
            }

            int new_preempt;

            // If no process found, default to null process (ensure it's in epoch)
            if (chosen_pid == NULLPROC) {
                proctab[NULLPROC].present_in_epoch = 1;
                proctab[NULLPROC].epoch = current_epoch;
            }

            // No switch needed if current process is chosen and still running
            if (chosen_pid == currpid && optr->pstate == PRCURR) {
                new_preempt = optr->counter;
                if(chosen_pid == NULLPROC){
                    new_preempt = QUANTUM;
                }
                #ifdef RTCLOCK
                    preempt = new_preempt;
                #endif
                return OK;
            }
            if (optr->pstate == PRCURR){
                optr->pstate = PRREADY;
                insert(currpid, rdyhead, optr->pprio);
            }
            

            // Dequeue chosen process from ready queue if it's there
            if (proctab[chosen_pid].pstate == PRREADY) {
                dequeue(chosen_pid);
            }

            // Switch to chosen process
            nptr = &proctab[chosen_pid];
            nptr->pstate = PRCURR;
            currpid = chosen_pid;

            #ifdef RTCLOCK
                preempt = nptr->counter;  // Set preempt to remaining counter
            #endif

            ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
            return OK;
        }
    }

    /* No switch needed if current process priority is higher than the next */
    if ((optr = &proctab[currpid])->pstate == PRCURR && lastkey(rdytail) < optr->pprio) {
        return OK;
    }

    /* Force context switch */
    if (optr->pstate == PRCURR) {
        optr->pstate = PRREADY;
        insert(currpid, rdyhead, optr->pprio);
    }

    /* Remove highest priority process at end of ready list */
    nptr = &proctab[(currpid = getlast(rdytail))];
    nptr->pstate = PRCURR;  /* mark it currently running */
    
    #ifdef RTCLOCK
        preempt = QUANTUM;  /* Reset preemption counter */
    #endif

    ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

    /* The OLD process returns here when resumed */
    return OK;
}
