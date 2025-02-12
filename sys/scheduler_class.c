#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lab1.h>

static int current_scheduler = AGESCHED; /* By default I am setting the current scheduler to AGESCHED*/

int current_epoch = 0; // This value will define the current running epoch to keep state

// This is my setter function for setting up for the scheduler

void setschedclass(int sched_class){
    int i;

    if (sched_class == AGESCHED || sched_class == LINUXSCHED){
        current_scheduler = sched_class;
    }

    if (sched_class == LINUXSCHED){

        /* For linux sched there are certain fields that i have included in the 
        proctab from where I can keep the logic stateful and include the proper process
        parameters*/

        /*I need to ensure that all the processes except the NULLPROC is set to have counter=0
        present_in_epoch as 0 and quantum as the priority considering that they have been just created 
        for the epoch and had never ran before. Also initializing their epoch value as current_epoch-1 just
        to ensure that they are not yet added to the current epoch */

        for (i = 1; i < NPROC; i++){
            struct pentry *p = &proctab[i];
            if(p->pstate == PRFREE)
                continue;
            p->counter = 0;
            p->present_in_epoch = 0;
            p->quantum = p->pprio;
            p->epoch = current_epoch-1;

        }

        /*Handling the NULLPROC in a different way, since NULLPROC will have 0 priority I need to set it to QUANTUM,
        Nullproc is always present in every epoch so marking it as present that is 1*/

        struct pentry *n = &proctab[NULLPROC];
        n->counter = 0;
        n->present_in_epoch = 1;
        n->quantum = QUANTUM;
        n->epoch = current_epoch;

    }
}


int getschedclass(void){
    return current_scheduler;
}