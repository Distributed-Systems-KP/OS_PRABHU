#ifndef _LAB1_H_
#define _LAB1_H_


#define AGESCHED 1
#define LINUXSCHED 2

extern int current_epoch; // global epoch state For LINUXSCHED
void setschedclass(int sched_class);
int getschedclass(void);

#endif