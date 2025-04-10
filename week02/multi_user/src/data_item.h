

#ifndef data_item_h
#define data_item_h

struct data_item_t {
    void *fifo_reserved;
    char msg[32];
} __attribute__ ((aligned (2)));

#endif
