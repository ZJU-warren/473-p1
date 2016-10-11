/**
 * 
 * File             : scheduler.c
 * Description      : This is a stub to implement all your scheduling schemes
 *
 * Author(s)        : @author
 * Last Modified    : @date
*/

// Include Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include <math.h>
#include <pthread.h>

void init_scheduler( int sched_type );
int schedule_me( float currentTime, int tid, int remainingTime, int tprio );
int num_preemeptions(int tid);
float total_wait_time(int tid);
void MLFQ_insert(PCB_List*);
void MLFQ_remove();
void preempt(PCB_List*);
*PCB_List search_blocked(int);


#define FCFS    0
#define SRTF    1
#define PBS     2
#define MLFQ    3

#define NUM_PRIO 5

//states for loc of PCB
#define NONE    0   //new thread
#define RDYQ    1   //in ready queue already
#define FINQ    2   //in blocked list (thread is coming back from being blocked)

//global variable for tracking which scheduling type to be used
int schedule_type; //INITIALIZE THIS

//node for PCB
//arrival_time and priority get updated when coming out of blocked queue
typedef struct PCB_list_
{
	int tid;		        //thread ID
	int loc;                //where this PCB is
	int num_preemptions;	//number of times this thread has been preempted
	float arrival_time;	    //most recent arrival time in the queue
	float time_waiting;	    //time this thread was waited in the ready queue
	int time_remaining;	    //time this thread has left before burst is done
	int priority;		    //priority the thread has
	struct PCB_list_* next;	//next PCB in the queue
} PCB_list;

//pointer to multi-level feedback queue
//will have 5 priority levels:
//for FCFS: insert at tail of first priority
//for SRTF: insert at first priority after any threads with less remaining time, and before any with more
//for PBS: insert at tail of appropriate priority
//for MLFQ: insert at tail of first priority, after time quanta up, insert at tail of one lower priority
PCB_list** MLFQ; //INITIALIZE THIS

//locks for maintaining atomicity when:
//inserting a thread into the ready queue
//removing a thread from the ready queue
//not sure if cpu_access needed but maybe?
pthread_mutex_t insert_lock; //INITIALIZE THIS
pthread_mutex_t remove_lock; //INITIALIZE THIS
pthread_mutex_t cpu_access_lock; //INITIALIZE THIS

//where the currently running thread's PCB is
PCB_list* running_thread;

//head pointer for the next thread
//gets moved to running_thread when running_thread is blocked/preempted
//updated on insertion or removal
PCB_list* next_thread;

//list for keeping track of threads that have already run but may run again
//search through here for number of preemptions and time waiting
PCB_list* blocked_list;

//this function will insert this_pcb into the queue at the appropriate spot
void MLFQ_insert(PCB_List *this_pcb) {

}

//this function will:
//remove the currently running thread,
//place it into the blocked_list,
//put the next_thread into running_thread,
//and find the next 
void MLFQ_remove() {

}

//this function will 
void preempt(PCB_List *this_pcb) {

}

//searches through stored PCB data for the thread
//returns pointer to that PCB
*PCB_List search_blocked(int tid) {
    PCB_List *search = blocked_list;
    
    //search for the appropriate thread
    while (search->tid != tid && search != NULL) {
        search = search->next;
    }
    
    return search;
}


void init_scheduler( int sched_type ) {
    schedule_type = sched_type;
    
    //initialize locks
    pthread_mutex_init(&insert_lock,NULL);
    pthread_mutex_init(&remove_lock,NULL);
    pthread_mutex_init(&cpu_access_lock,NULL);
    
    //initalize MLFQ
    MLFQ = malloc(NUM_PRIO * sizeof(PCB_list*));
    for (int i = 0; i < NUM_PRIO; i++) {
        MLFQ[i] = NULL;
    }
    
    running_thread = NULL;
    next_thread = NULL;
    blocked_list = NULL;
}

int schedule_me( float currentTime, int tid, int remainingTime, int tprio ) {
	

	//wait for this PCB to placed into the running thread
	while (running_thread->tid != tid) {
		sched_yield();
	}

	//increment the wait time
	this_pcb->time_waiting += (get_global_time() - this_pcb->arrival_time);

	//if this thread is done, remove it from the ready queue, insert in 
	if (remainingTime == 0) {
		MLFQ_remove(&this_pcb);
	}

	//return the global time
	return get_global_time(); //not sure if this is correct
}

int num_preemeptions(int tid){
	//search for the PCB
	PCB_List *this_pcb = search_blocked(tid);

	//if PCB exists, return the number of preemptions
	if (this_pcb != NULL) {
		return this_pcb->num_preemptions;
	} 

	//otherwise, return -1
	else {
	return -1;
	}
}

float total_wait_time(int tid){
	//search for the PCB
	PCB_List *this_pcb = search_blocked(tid);

	//if PCB exists, return the number of preemptions
	if (this_pcb != NULL) {
		return this_pcb->time_waiting;
	} 

	//otherwise, return -1
	else {
	return -0.1;
	}
}
