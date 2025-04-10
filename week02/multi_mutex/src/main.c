
#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/random/random.h>


// use k_thread_create() to make threads dynamically,
// or use K_THREAD_DEFINE() to make threads at compile time
// the difference between DEFINE and create also applies to 
// synchronization things like semaphores and mutexes!


// create mutex only argument is name of newly defined mutex
K_MUTEX_DEFINE(mutex_0);
K_MUTEX_DEFINE(mutex_1);


// set number of threads, sizes of thir stacks, and priorities
#define NUM_THREADS 2
#define STACK_SIZE 512
#define THREAD_0_PRIO -2
#define THREAD_1_PRIO 5

#define TOTAL 40
volatile int inc_count = 0;
volatile int dec_count = 40;

// code to be executed by two threads, the stuff between the call to k_mutex_lock and
// k_mutex_unlock needs to be done atomically, otherwise the sum of inc and dec_count
// will be thrown off of the total
void multithreaded(void) {
    // get thread id for later print statements
    k_tid_t tid = k_current_get();

    // sleep for a bit so the prints are sent at a humanly readable speed
    k_msleep(3000);

    // kinda self explanatory, lock the mutex called mutex_0
    // second argument specifies how long to keep it locked for
    // other options K_CYC(t) which times out after t clock cycles
    // or K_HOURS(h), K_MINUTES(m), etc where it waits for however many hours or minutes
    // the -EAGAIN value is returned if the wait period times out
    if (k_mutex_lock(&mutex_0, K_SECONDS(5)) != -EAGAIN) {
        printk("Thread %d modifying counts\n", tid);
        k_msleep(6000);
        inc_count++;
        inc_count = inc_count % TOTAL;
        dec_count--;

        if (dec_count == 0) {
            dec_count = TOTAL;
        }
        // kinda self explanatory, unlock the mutex called mutex_0
        k_mutex_unlock(&mutex_0);
    } else {
        // lots of exclamation points to make it easy to see in putty
        printk("Thread %d timed out!!!!!!!!!!!!!!!!!!!!!!!\n", tid);
    }
    

    if (inc_count + dec_count != TOTAL) {
        printk("Race condition!\n");
        //printk("inc_count (%d) + dec_count (%d) = (%d) \n", inc_count, dec_count
        //    (inc_count + dec_count)); //can printk not take more than one format input argument thing??
        printk("inc_count (%d) ", inc_count);
        printk("dec_count (%d) ", dec_count);
        printk("sum (%d)\n", inc_count + dec_count);
        k_msleep(400 + sys_rand32_get() % 10);
    }
}

// thread 0 --> priority -2
void thread_0(void) {

    //get thread id
    k_tid_t tid = k_current_get();

    // say hello
    printk("Thread 0 started with id %d\n", tid);

    while (1) {
        // call the same function as thread 1 ad infinitum
        multithreaded();
        // let the computer know
        printk("Thread 0 iterated\n");
        // sleep a little to make things unbalanced between the threads
        k_msleep(500);
    }
}

// thread 1 --> priority 5
void thread_1(void) {

    k_tid_t tid = k_current_get();

    printk("Thread 1 started with id %d\n", tid);

    while (1) {
        multithreaded();
        printk("Thread 1 iterated\n");
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
    THREAD_0_PRIO, 0, 5000);
K_THREAD_DEFINE(thread_1_id, STACK_SIZE, thread_1, NULL, NULL, NULL,
    THREAD_1_PRIO, 0, 4990);

