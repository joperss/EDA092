/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

#include "devices/batch-scheduler.c"



void test_batch_scheduler(void)
{
    init_bus();
    batchScheduler(0, 0, 0, 0);
    // printf("done\n");
    batchScheduler(1, 0, 0, 0);
    // printf("done\n");
    batchScheduler(0, 0, 0, 1);
    // printf("done\n");
    batchScheduler(0, 4, 0, 0);
    // printf("done\n");
    batchScheduler(0, 0, 4, 0);
    // printf("done\n");
    batchScheduler(3, 3, 3, 3);
    // printf("done\n");
    batchScheduler(4, 3, 4 ,3);
    // printf("done\n");
    batchScheduler(7, 23, 17, 1);
    // printf("done\n");
    batchScheduler(40, 30, 0, 0);
    // printf("done\n");
    batchScheduler(30, 40, 0, 0);
    // printf("done\n");
    batchScheduler(23, 23, 1, 11);
    // printf("done\n");
    batchScheduler(22, 22, 10, 10);
    // printf("done\n");
    batchScheduler(0, 0, 11, 12);
    // printf("done\n");
    batchScheduler(0, 10, 0, 10);
    // printf("done\n");
    batchScheduler(0, 10, 10, 0);
    pass();
}
