
#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/random/random.h>

# include <string.h>

// create mutex only argument is name of newly defined mutex
K_MUTEX_DEFINE(mutex_0);
K_MUTEX_DEFINE(mutex_1);
K_MUTEX_DEFINE(mutex_2);

// create fifo only argument is name of newly defined fifo
K_FIFO_DEFINE(msgFifo);

// set sizes of thir stacks and thread priorities
#define STACK_SIZE 512
#define THREAD_0_PRIO 3
#define THREAD_1_PRIO 5
#define THREAD_2_PRIO 5

struct k_stack stack1;
struct k_stack stack2;

#define K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
#define K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

void thread_user(void);

// thread 0 --> supervisor mode thread
int main(void) {
    printk("HELLO!0\n");

    k_fifo_init(&msgFifo);

    struct k_thread thread_st_1;
    struct k_thread thread_st_2;
    printk("HELLO!1\n");

    k_tid_t tid_1 = k_thread_create(&thread_st_1, &stack1, K_KERNEL_STACK_SIZEOF(stack1), thread_user, NULL, NULL, NULL, THREAD_1_PRIO, K_USER, K_MSEC(100));
    k_tid_t tid_2 = k_thread_create(&thread_st_2, &stack2, K_KERNEL_STACK_SIZEOF(stack2), thread_user, NULL, NULL, NULL, THREAD_1_PRIO, K_USER, K_MSEC(100));
    printk("HELLO!2\n");
    k_object_access_grant(&msgFifo, tid_1);
    k_object_access_grant(&msgFifo, tid_2);
    printk("HELLO!3\n");

    char* msg = NULL;

    while (1) {
        msg = k_fifo_get(&msgFifo, K_NO_WAIT);
        if (msg != NULL) {
            printk("%s\n", msg);
        }
        k_msleep(50);
    }

    return 1;
}

// thread 1 --> user mode thread
void thread_user(void) {
    char msg[32];
    memset(msg, 0, sizeof(msg));

    while (1) {
        for (uint8_t i = 0; i < 31; i++) {
            memset(
                msg + i,
                ((sys_rand8_get() % 89) + 33),
                1
            );
        }
        msg[31] = '\0';

        k_fifo_alloc_put(&msgFifo, &msg);

        k_msleep(10000);
    }
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