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

//node for PCB
//arrival_time and priority get updated when coming out of blocked queue
//tq_remaining will be used to determine when to demote this thread to a lower MLFQ priority
typedef struct PCB_List_
{
	int tid; //thread ID
	int num_preemptions; //number of times this thread has been preempted
	float arrival_time; //most recent arrival time in the queue
	float time_waiting; //time this thread was waited in the ready queue
	int time_remaining; //time this thread has left before burst is done
	int priority; //priority the thread has
	int tq_remaining; //how long is left for this time quantum
	struct PCB_List_* next;	//next MFLQ_list[CB in the queue
} PCB_List;

void init_scheduler( int sched_type );
int schedule_me( float currentTime, int tid, int remainingTime, int tprio );
int num_preemeptions(int tid);
float total_wait_time(int tid);

void MLFQ_insert(PCB_List*);
void MLFQ_remove();
PCB_List* blocked_remove(int);
void blocked_insert(PCB_List*);
void running_thread_update();
void MLFQ_demote(PCB_List*);
void advance_scheduler_time(float);



#define FCFS    0
#define SRTF    1
#define PBS     2
#define MLFQ_id 3

#define NUM_PRIO 5

//global variable for tracking which scheduling type to be used
int schedule_type;

//pointer to multi-level feedback queue
//will have 5 priority levels
PCB_List** MLFQ;

//locks for maintaining atomicity when:
//updating MLFQ
//updating blocked_list
//updating running_thread
pthread_mutex_t* MLFQ_lock;
pthread_mutex_t* blocked_list_lock;
pthread_mutex_t* running_thread_lock;
pthread_mutex_t* global_time_lock;

//where the currently running thread's PCB is
PCB_List* running_thread;

//list for keeping track of threads that have already run but may run again
//search through here for number of preemptions and time waiting
PCB_List* blocked_list;

//global time tracker, with global_time_lock
int scheduler_time = 0;

//this function will insert this_pcb into the queue at the appropriate spot:
//for FCFS: insert at tail of first priority
//for SRTF: insert at first priority after any threads with less remaining time
//for PBS: insert at appropriate priority, after any threads with less remaining time
//for MLFQ: insert at tail of first priorityy
void MLFQ_insert(PCB_List *this_pcb) {
	PCB_List *search = blocked_remove(this_pcb->tid); //will search for and remove PCB in blocked list
	if (search != NULL) { //thread is coming from blocked queue
		//update number preemptions and arrival time
		this_pcb->num_preemptions = search->num_preemptions;
		this_pcb->arrival_time = search->arrival_time;
	}
	
	while (pthread_mutex_trylock(MLFQ_lock) != 0); //lock
	
	if (schedule_type == FCFS) {
		search = MLFQ[0]; //point to head of queue
		if (search == NULL) {//base case
			MLFQ[0] = this_pcb;
		}
		else{
			//search for tail
			while (search->next != NULL) {
				search = search->next;
			}
			
			//insert
			search->next = this_pcb;
		}
		
	} else if (schedule_type == SRTF) {
		search = MLFQ[0]; //point to head of queue
		{
			//search for tail or a thread that has a longer time remaining
			//if (search->time_remaining < this_pcb->time_remaining)
			while ((search != NULL) && (search->time_remaining < this_pcb->time_remaining)) {
				search = search->next;
			}
			
			//insert
			if (search == MLFQ[0]){
				this_pcb->next = search;
				MLFQ[0] = this_pcb;
			}
			else{
				this_pcb->next = search;
				search = this_pcb;
			}

		}
		
	} else if (schedule_type == PBS) {
		search = MLFQ[this_pcb->priority - 1]; //point to appropriate priority
		if (search == NULL) //base case
			MLFQ[this_pcb->priority - 1] = this_pcb;
		else{
			//search for tail or a thread that has a longer time remaining
			while ((search->next != NULL) && (search->next->time_remaining < this_pcb->time_remaining)) {
				search = search->next;
			}
			
			//insert
			this_pcb->next = search->next;
			search->next = this_pcb;
		}
		
	} else { //MLFQ
		search = MLFQ[0]; //point to head of queue
		if (search == NULL) //base case
			MLFQ[0] = this_pcb;
		else{
			//search for tail
			while (search->next != NULL) {
				search = search->next;
			}
			
			//insert
			search->next = this_pcb;
			
			//set TQ for this thread to 5 (TQ for highest priority)
			search->tq_remaining = 5;
		}
	}
	
	//update running_thread in case of preemption
	running_thread_update();
	
	pthread_mutex_unlock(MLFQ_lock); //unlock
}

//this function will:
//remove the currently running thread,
//place it into the blocked_list,
//put the next_thread into running_thread,
//and find the next 
void MLFQ_remove() {
	while(pthread_mutex_trylock(MLFQ_lock) != 0); //lock
	PCB_List *remove = running_thread;
	
	//update MLFQ priority pointers
	if ((schedule_type == FCFS) || (schedule_type == SRTF)) {
		MLFQ[0] = running_thread->next;
	} else { //PBS or MLFQ
		MLFQ[running_thread->priority - 1] = running_thread->next;
	}
	
	//update running_thread
	running_thread_update();
	
	//update next
	remove->next = NULL;
	
	//decrement preemptions (running_thread_update automatically increments)
	remove->num_preemptions--;
	
	//insert finished thread into blocked_list
	blocked_insert(remove);
	
	pthread_mutex_unlock(MLFQ_lock); //unlock
}

//inserts a PCB into the blocked_list
void blocked_insert(PCB_List* insert) {
	//lock
	while (pthread_mutex_trylock(blocked_list_lock) != 0);
	
	PCB_List *temp = blocked_list;
	if (temp == NULL){
		blocked_list = insert;
	}
	else{
		while (temp->next != NULL) {
			temp = temp->next;
		}
		
		temp->next = insert;
		insert->next = NULL;
	}
	
	//unlock
	pthread_mutex_unlock(blocked_list_lock);
}

//searches through stored PCB data for the thread
//returns pointer to that PCB, or NULL if not in blocked_list
PCB_List* blocked_remove(int tid) {
    //if blocked_list is empty
	if (blocked_list == NULL) {
		return NULL;
	}
	
	//lock
	while (pthread_mutex_trylock(blocked_list_lock) != 0);
	
	PCB_List *search = blocked_list;
    
    //search for the appropriate thread
    while ((search->next != NULL) && (search->next->tid != tid)) {
        search = search->next;
    }
	
	//if the thread isn't in blocked_list
	if (search->next == NULL) {
		pthread_mutex_unlock(blocked_list_lock); //unlock
		return NULL;
	} else { //thread found
		PCB_List *found = search->next;
		search->next = found->next;
		pthread_mutex_unlock(blocked_list_lock); //unlock
		return found;
	}
}

//used for finding the proper process to run
void running_thread_update() {
	while(pthread_mutex_trylock(running_thread_lock) != 0); //lock
	
	//keep track of current running_thread in case of preemption
	PCB_List *old = running_thread;
	
	if ((schedule_type == FCFS) || (schedule_type == SRTF)) {
		running_thread = MLFQ[0];
	} else { //same for PBS and MLFQ, search for highest priority that isn't NULL
		int i;
		for (i = (NUM_PRIO - 1); i > 0; i--) { //search from lowest to highest
			if (MLFQ[i] != NULL) {
				running_thread = MLFQ[i];
			}
		}
	}
	
	//if the old thread is not the current thread, increment preemptions
	if (running_thread == NULL || (old != NULL && old->tid != running_thread->tid)) {
		old->num_preemptions++;
	}
	
	pthread_mutex_unlock(running_thread_lock); //unlock
}

//used for demoting a thread after finishing it's current time quantum
void MLFQ_demote(PCB_List* this_pcb) {
	while (pthread_mutex_trylock(MLFQ_lock) != 0); //lock
	
	//set this priority's head pointer to this->next
	MLFQ[this_pcb->priority - 1]->next = this_pcb->next;
	
	//set to next priority, if not at lowest
	if (this_pcb->priority != (NUM_PRIO - 1)) {
		this_pcb->priority++;
	}
	
	//NULL this next
	this_pcb->next = NULL;
	
	//enqueue at end of new priority
	PCB_List *search = MLFQ[this_pcb->priority - 1];
	while (search->next != NULL) {
		search = search->next;
	}
	search->next = this_pcb;
	
	//increment num_preemptions
	this_pcb->num_preemptions++;
	
	//update running_thread
	running_thread_update();
	
	pthread_mutex_unlock(MLFQ_lock); //unlock
}

void init_scheduler( int sched_type ) {
    schedule_type = sched_type;
    
    //initialize locks
    MLFQ_lock = malloc(sizeof(pthread_mutex_t)); 
    blocked_list_lock = malloc(sizeof(pthread_mutex_t)); 
    running_thread_lock = malloc(sizeof(pthread_mutex_t)); 
    global_time_lock = malloc(sizeof(pthread_mutex_t)); 
 
    pthread_mutex_init(MLFQ_lock, NULL); 
    pthread_mutex_init(blocked_list_lock, NULL); 
  	pthread_mutex_init(running_thread_lock, NULL); 
  	pthread_mutex_init(global_time_lock, NULL); 
    
    //initalize MLFQ
    MLFQ = malloc(NUM_PRIO * sizeof(PCB_List*));
    int i;
    for (i = 0; i < NUM_PRIO; i++) {
        MLFQ[i] = NULL;
    }
    
    running_thread = NULL;
    blocked_list = NULL;
}

int schedule_me( float currentTime, int tid, int remainingTime, int tprio ) {
	PCB_List *this_pcb = blocked_remove(tid);
	
	//check if thread already running, if so, update remainingTime
	if ((running_thread != NULL) && (running_thread->tid == tid)) {
		running_thread->time_remaining = remainingTime;
		
	} else if (this_pcb != NULL){ //else, check if coming from blocked queue
		//if so, update arrival_time, time_remaining, priority
		this_pcb->arrival_time = currentTime;
		this_pcb->time_remaining = remainingTime;
		this_pcb->priority = tprio;
		
	} else { //else new thread, create pcb
		this_pcb = malloc(sizeof(PCB_List));
		this_pcb->tid = tid;
		this_pcb->num_preemptions = 0;
		this_pcb->arrival_time = currentTime;
		this_pcb->time_waiting = 0;
		this_pcb->time_remaining = remainingTime;
		this_pcb->priority = tprio;
		this_pcb->tq_remaining = 0;
		this_pcb->next = NULL;
	}
	
	//if thread not already running, insert into ready queue
	if ((running_thread == NULL) || (running_thread->tid != tid)) {
		MLFQ_insert(this_pcb);
	}
	
	//if MFLQ, if TQ done, demote
	if ((schedule_type == MLFQ_id) && (this_pcb->tq_remaining == 0)) {
		MLFQ_demote(this_pcb);
	}
	

	//wait for this PCB to placed into the running thread
	while ((running_thread == NULL) || (running_thread->tid != tid)) {
		sched_yield();
	}
	
	//--------------------
	//AT THIS POINT THIS THREAD HAS CPU
	//--------------------
	
	//if MLFQ, update TQ
	if (schedule_type == MLFQ_id) {
		this_pcb->tq_remaining--;
	}

	//increment the wait time
	advance_scheduler_time(currentTime);
	running_thread->time_waiting += (scheduler_time - currentTime);

	//if this thread is done, remove it from the ready queue, insert in 
	if (remainingTime == 0) {
		MLFQ_remove(&this_pcb);
		scheduler_time -= 1;
	}

	//return the global time
	int time = scheduler_time;
	return time; //not sure if this is correct
}

int num_preemeptions(int tid){
	//search for the PCB

	PCB_List *search = blocked_list;
	if (search->tid == tid){ //if first term is it
		return search->num_preemptions;
	} else {
		while ((search->next != NULL) && (search->next->tid != tid)) {
        search = search->next;
    	}
	}
    if (search->next != NULL) {
    	return search->next->num_preemptions;
    } else {
    	return -2;
    }
}

float total_wait_time(int tid){
	PCB_List *search = blocked_list;
	if (search->tid == tid){ //if first term is it
		return search->time_waiting;
	} else {
		while ((search->next != NULL) && (search->next->tid != tid)) {
        search = search->next;
    	}
	}
    if (search->next != NULL) {
    	return search->next->time_waiting;
    } else {
    	return -2;
    }
}

void advance_scheduler_time(float next_arrival)
{
	pthread_mutex_lock(global_time_lock);
		float next_ms = floor(scheduler_time + 1.0);
		if ((next_arrival < next_ms) && (next_arrival > 0))
			scheduler_time = next_ms;
		else
			scheduler_time = next_arrival;
	pthread_mutex_unlock(global_time_lock);
}