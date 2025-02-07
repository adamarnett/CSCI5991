
#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/random/random.h>

# include <string.h>

# include "data_item.h"

#include <zephyr/debug/gdbstub.h>

// create mutex only argument is name of newly defined mutex
K_MUTEX_DEFINE(mutex_0);
K_MUTEX_DEFINE(mutex_1);
K_MUTEX_DEFINE(mutex_2);

// create fifo only argument is name of newly defined fifo
K_FIFO_DEFINE(msgFifo);

// set sizes of thir stacks and thread priorities
#define STACK_SIZE 512
#define THREAD_0_PRIO 60
#define THREAD_1_PRIO 5
#define THREAD_2_PRIO 5

//struct k_thread_stack stack1;
//struct k_thread_stack stack2;

// stacks need to be defined before k_thread_create can be called
K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

// user mode thread
// void pointer arguments satisfy an argument type in k_thread_create, don't know how to suppress the warning
void thread_user(void* v0, void* v1, void* v2) {

    printk("begin user thread!\n");

    struct data_item_t tx;

    while (1) {
        memset(tx.msg, 0, sizeof(char)*32);
        for (uint8_t i = 0; i < 31; i++) {
            memset(
                tx.msg + i,
                ((sys_rand8_get() % 89) + 33),
                1
            );
            //printk("[%s]\n", tx.msg);
        }
        tx.msg[31] = '\0';

        printk("tx.msg right before fifo_put: [%s]\n", tx.msg);
        k_fifo_put(&msgFifo, &tx);
        
        k_msleep(10000);
    }
}

// main --> supervisor mode thread
int main(void) {
    printk("HELLO!0\n");

    k_fifo_init(&msgFifo);

    struct k_thread thread_st_1;
    //struct k_thread thread_st_2;

    k_tid_t tid_1 = k_thread_create(&thread_st_1, stack1, K_KERNEL_STACK_SIZEOF(stack1), thread_user, NULL, NULL, NULL, THREAD_1_PRIO, 0, K_MSEC(10000));
    //k_tid_t tid_2 = k_thread_create(&thread_st_2, stack2, K_KERNEL_STACK_SIZEOF(stack2), thread_user, NULL, NULL, NULL, THREAD_1_PRIO, K_USER, K_MSEC(100));
    //k_object_access_grant(&msgFifo, tid_1);
    //k_object_access_grant(&msgFifo, tid_2);

    struct data_item_t* rx = NULL;

    while (1) {
        printk("Before fifo_get\n");
        // recieve item from the fifo
        rx = k_fifo_get(&msgFifo, K_FOREVER);
        printk("rx->msg after fifo_get: [%s]\n", rx->msg);
        if (rx->msg != NULL) {
            printk("%s\n", rx->msg);
        }
    }

    return 1;
}



// thread 2 --> user mode thread
//void thread_2(void) {
//
//}

// define threads
// thread 0 is a supervisor mode thread (full access to all kernel objects)
// thread 1 & 2 is a user mode thread (no access to kernel objects)
//K_THREAD_DEFINE(thread_0_id, STACK_SIZE, thread_0, NULL, NULL, NULL,
//    THREAD_0_PRIO, 0, 0);
//K_THREAD_DEFINE(thread_1_id, STACK_SIZE, thread_1, NULL, NULL, NULL,
//    THREAD_1_PRIO, K_USER, 100);
//K_THREAD_DEFINE(thread_2_id, STACK_SIZE, thread_1, NULL, NULL, NULL,
//    THREAD_2_PRIO, K_USER, 100);
