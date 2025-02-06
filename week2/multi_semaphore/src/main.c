
#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

//#include <zephyr/random/random.h>

// use k_thread_create() to make threads dynamically,
// or use K_THREAD_DEFINE() to make threads at compile time


// set number of threads, sizes of thir stacks, and priorities
#define NUM_THREADS 4
#define STACK_SIZE 512
#define THREAD_0_PRIO 3
#define THREAD_1_PRIO 5
#define THREAD_2_PRIO 5
#define THREAD_3_PRIO 5

// define the semaphore
// first arg is the semaphore itself
// second is the initial count
// third is the limit of the count
K_SEM_DEFINE(resource_sem, 10, 15);

// define resources (just an int for lack of actual peripheral or memory or something)
volatile uint32_t available_resources = 10;

void release_resource(void) {
    //if (available_resources < 15) {
    //    available_resources++;
    //}
    k_sem_give(&resource_sem);
    available_resources++;
    printk("Resource created! Current count: %d\n\n", available_resources);
}

void acquire_resource(void) {
    k_sem_take(&resource_sem, K_FOREVER);
    available_resources--;
    printk("Resource consumed! Current count: %d\n\n", available_resources);
}

void thread_0(void) {
    while (1) {
        k_tid_t tid = k_current_get();
        printk("Thread 0 (%d) producing one resource...\n", tid);

        release_resource();

        //k_msleep(500 + sys_rand32_get() % 10);
        k_msleep(10000);

    }
}

void thread_1(void) {
    while (1) {
        k_tid_t tid = k_current_get();
        printk("Thread 1 (%d) consuming one resource...\n", tid);

        acquire_resource();

        //k_msleep(sys_rand32_get() % 10);
        k_msleep(5000);

    }
}

void thread_2(void) {
    while (1) {
        k_tid_t tid = k_current_get();
        printk("Thread 2 (%d) consuming three resources...\n", tid);

        acquire_resource();
        acquire_resource();
        acquire_resource();

        //k_msleep(sys_rand32_get() % 10);
        k_msleep(20000);

    }
}

void thread_3(void) {
    while (1) {
        k_tid_t tid = k_current_get();
        printk("Thread 3 (%d) producing five resources...\n", tid);

        k_msleep(30000);

        release_resource();
        release_resource();
        release_resource();
        release_resource();
        release_resource();

    }
}

void main(void) {

    printk("\n\n\nBEGIN!\n\n\n");

    while (1) {
        printk(".\n\n");
        k_msleep(1000);
    }

}


// first arg is the new thread itself (which we are defining)
// second is stack size
// third is entry function (function to run with the thread)
// fourth, fifth, and sixth are arguments that can be passed to the thread's entry function
// seventh is the thread priority (lower number = more priority)
//      priority can be negative, in that case that thread will continue until it gives up control
// eigth is options such as K_ESSENTIAL which makes the exiting of the thread a fatal error
// ninth and last is the delay (milliseconds) between the calling of the function and intended start of the thread
K_THREAD_DEFINE(thread_0_id, STACK_SIZE, thread_0, NULL, NULL, NULL,
    THREAD_0_PRIO, 0, 6000);
K_THREAD_DEFINE(thread_1_id, STACK_SIZE, thread_1, NULL, NULL, NULL,
    THREAD_1_PRIO, 0, 6000);
K_THREAD_DEFINE(thread_2_id, STACK_SIZE, thread_2, NULL, NULL, NULL,
    THREAD_2_PRIO, 0, 6000);
K_THREAD_DEFINE(thread_3_id, STACK_SIZE, thread_3, NULL, NULL, NULL,
    THREAD_3_PRIO, 0, 6000);

