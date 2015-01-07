/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

struct semaphore busSema;
struct semaphore HighPriority;
struct semaphore senders;
struct semaphore receivers;
struct lock hplock;
volatile int direction = 0;
volatile int HP = 1;
//struct semaphore priosend;
//struct semaphore receivers;
//struct semaphore priorecv;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */



/* initializes semaphores */ 
void init_bus(void){ 
 
    random_init((unsigned int)123456789); 
    sema_init(&busSema, BUS_CAPACITY);
    sema_init(&HighPriority, 20);
    sema_init(&senders, BUS_CAPACITY);
    sema_init(&receivers, BUS_CAPACITY);
    lock_init(&hplock);
//    sema_init(&priosend, BUS_CAPACITY);
//    sema_init(&priorecv, BUS_CAPACITY);

}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
    int i;
    char *name1 = "pri_send";
    for(i = 0; i < num_priority_send; i++) {
        thread_create(name1, 0, senderPriorityTask, 0);
    }
    char *name2 = "pri_recv";
    for(i = 0; i < num_priority_receive; i++) {
        thread_create(name2, 0, receiverPriorityTask, 0);
    }
    char *name3 = "send";
    for(i = 0; i < num_tasks_send; i++) {
        thread_create(name3, 0, senderTask, 0);
    }
    char *name4 = "recv";
    for(i = 0; i < num_task_receive; i++) {
        thread_create(name4, 0, receiverTask, 0);
    }
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
    if (task.priority == HIGH) {
        sema_down(&HighPriority);
        if (HP == 0) {
            HP = 1;
        }
        if (task.direction == SENDER) {
            sema_down(&senders);
            while(direction == 1) timer_msleep(10);
        }
        else {
            sema_down(&receivers);
            while(direction == 0) timer_msleep(10);
        }
        sema_up(&HighPriority);
        if (list_empty(&HighPriority.waiters) && HighPriority.value == 20) {
            HP = 0;
        }
    }
    else {
        while (HP == 1) timer_msleep(10);
        if (task.direction == SENDER) {
            sema_down(&senders);
            while(direction == 1);
        }
        else {
            sema_down(&receivers);
            while (direction == 0);
        }
    }
    sema_down(&busSema);
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
    //printf("Transferring data\n");
    timer_msleep(random_ulong()%100);
    //printf("Transfer done\n");
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
    sema_up(&busSema);
    if (task.direction == SENDER) {
        sema_up(&senders);
        if (list_empty(&senders.waiters) && senders.value == BUS_CAPACITY)
        {
            direction = 1;
        }
    }
    else {
        sema_up(&receivers);
        if (list_empty(&receivers.waiters) && receivers.value == BUS_CAPACITY)
        {
            direction = 0;
        }
    }
}