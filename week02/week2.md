## Week 2 - Multithreading


#### multi_mutex
Demonstration of usage of a mutex - two threads increment and decrement two variables in such a way that a race condition could happen extremely easily (if the mutex is removed that happens immediately). Started this by doing an exercise from the Nordic Semiconductor tutorial and expanded/modified it a bit.

#### multi_semaphore
Demonstration of usage of a semaphore - two threads consume resources at a rate faster than the two threads that produce them. Shows that the consumer threads end up being forced to wait for the producer threads to create the resources. Also started out as a Nordic Semiconductor tutorial, but expanded a bunch.

#### multi_user
Attempted demonstration of usage of FIFOs and userspace threads. Seems to lock up when the user thread first tries to pass something to the FIFO. Might be due to lack of access to the FIFO, since the thread is created (as a user thread) and the access is granted after its creation. I tried increasing the delay on when the thread is allowed to start so that the supervisor thread can give access to the user thread, but I'm not testing it immediately because Im in a coffee shop and don't want to take my board out right now.
Last night I tried to get gdb working to debug this, but I couldn't quite get it. Then I tried QEMU, which was even less successful...
