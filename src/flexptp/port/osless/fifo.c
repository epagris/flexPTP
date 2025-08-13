#include "fifo.h"
#include <string.h>

void fifo_init(Fifo *f, uint32_t len, uint32_t esize, uint8_t *data, FifoLockFn lockFn) {
    f->len = len;
    f->esize = esize;
    f->data = data;
    f->read = 0;
    f->write = 0;
    f->level = 0;
    f->lockFn = lockFn;
}

#define FIFO_GET_ELEMENT_PTR(f, i) ((void *)(f->data + (f->esize * i)))
#define FIFO_ADVANCE_INDEX(f, k) ((k) >= ((f)->len - 1)) ? 0 : ((k) + 1)

bool fifo_push(Fifo * f, void const * item) {
    // the storage is full
    if (f->level == f->len) {
        return false;
    }

    // enter critical section
    f->lockFn(true);

    // get destination pointer
    void * dst = FIFO_GET_ELEMENT_PTR(f, f->write);

    // store the data
    memcpy(dst, item, f->esize);

    // increment utilization
    f->level++;

    // advance write index
    f->write = FIFO_ADVANCE_INDEX(f, f->write);

    // leave critical section
    f->lockFn(false);

    return true;
}

uint32_t fifo_get_level(const Fifo * f) {
    return f->level;
}

bool fifo_pop(Fifo * f, void * item) {
        // the storage is full
    if (f->level == 0) {
        return false;
    }

    // enter critical section
    f->lockFn(true);

    // get source pointer
    void * src = FIFO_GET_ELEMENT_PTR(f, f->read);

    // read the data
    memcpy(item, src, f->esize);

    // decrement utilization
    f->level--;

    // advance read index
    f->read = FIFO_ADVANCE_INDEX(f, f->read);

    // leave critical section
    f->lockFn(false);

    return true;
}

void fifo_clear(Fifo * f) {
    // enter critical section
    f->lockFn(true);

    // reset level and indices
    f->level = 0;
    f->read = 0;
    f->write = 0;

    // leave critical section
    f->lockFn(false);
}