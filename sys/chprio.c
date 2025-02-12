/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lab1.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pprio = newprio;
	
	if(getschedclass() == LINUXSCHED){
		pptr->present_in_epoch =0;
	}
	else{
		if (pptr->pstate == PRREADY){
			dequeue(pid);
			insert(pid, rdyhead, newprio);
		}
	}
	restore(ps);
	return(newprio);
}
