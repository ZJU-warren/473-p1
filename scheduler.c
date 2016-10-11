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
*PCB_List blocked_remove(int);
void blocked_insert(PCB_List*);
void MLFQ_demote(PCB_List*);


#define FCFS    0
#define SRTF    1
#define PBS     2
#define MLFQ    3

#define NUM_PRIO 5

//global variable for tracking which scheduling type to be used
int schedule_type; //INITIALIZE THIS

//node for PCB
//arrival_time and priority get updated when coming out of blocked queue
//tq_remaining will be used to determine when to demote this thread to a lower MLFQ priority
typedef struct PCB_list_
{
	int tid; //thread ID
	int num_preemptions; //number of times this thread has been preempted
	float arrival_time; //most recent arrival time in the queue
	float time_waiting; //time this thread was waited in the ready queue
	int time_remaining; //time this thread has left before burst is done
	int priority; //priority the thread has
	int tq_remaining; //how long is left for this time quantum
	struct PCB_list_* next;	//next PCB in the queue
} PCB_list;

//pointer to multi-level feedback queue
//will have 5 priority levels
PCB_list** MLFQ; //INITIALIZE THIS

//locks for maintaining atomicity when:
//updating MLFQ
//updating blocked_list
pthread_mutex_t MLFQ_lock; //initialize this
pthread_mutex_t blocked_list_lock; //intialize this

//where the currently running thread's PCB is
PCB_list* running_thread;

//list for keeping track of threads that have already run but may run again
//search through here for number of preemptions and time waiting
PCB_list* blocked_list;

//this function will insert this_pcb into the queue at the appropriate spot:
//for FCFS: insert at tail of first priority
//for SRTF: insert at first priority after any threads with less remaining time, and before any with more
//for PBS: insert at tail of appropriate priority
//for MLFQ: insert at tail of first priority, after time quanta up, insert at tail of one lower priority
void MLFQ_insert(PCB_List *this_pcb) {
	
}

//this function will:
//remove the currently running thread,
//place it into the blocked_list,
//put the next_thread into running_thread,
//and find the next 
void MLFQ_remove() {

}

//inserts a PCB into the blocked_list
void blocked_insert(PCB_List* insert) {
	while (pthread_mutex_trylock(blocked_list_lock) != 0);
	
	PCB_List *temp = blocked_list;
	
	while (temp->next != NULL) {
		temp = temp->next;
	}
	
	temp->next = insert;
	
	pthread_mutex_unlock(blocked_list_lock);
)

//searches through stored PCB data for the thread
//returns pointer to that PCB, or NULL if not in blocked_list
*PCB_List blocked_remove(int tid) {
    //if blocked_list is empty
	if (blocked_list->next == NULL) {
		return NULL;
	}
	
	PCB_List *search = blocked_list;
    
    //search for the appropriate thread
    while ((search->next->tid != tid) && (search->next != NULL)) {
        search = search->next;
    }
	
	//if the thread isn't in blocked_list
	if (search->next == NULL) {
		return NULL;
	} else { //thread found
		PCB_List *found = search->next;
		search->next = found->next;
		return found;
	}
}

//used for demoting a thread after finishing it's current time quantum
void MLFQ_demote(PCB_List* this_pcb) {
	
}

void init_scheduler( int sched_type ) {
    schedule_type = sched_type;
    
    //initialize locks
    pthread_mutex_init(&MLFQ_lock,NULL);
    pthread_mutex_init(&blocked_list_lock,NULL);
    
    //initalize MLFQ
    MLFQ = malloc(NUM_PRIO * sizeof(PCB_list*));
    for (int i = 0; i < NUM_PRIO; i++) {
        MLFQ[i] = NULL;
    }
    
    running_thread = NULL;
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
	PCB_List *this_pcb = blocked_remove(tid);

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
	PCB_List *this_pcb = blocked_remove(tid);

	//if PCB exists, return the number of preemptions
	if (this_pcb != NULL) {
		return this_pcb->time_waiting;
	} 

	//otherwise, return -1
	else {
	return -0.1;
	}
}
