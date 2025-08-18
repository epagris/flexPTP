#ifndef OSLESS_FIFO
#define OSLESS_FIFO

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FIFO lock function.
 * 
 * @param lock lock or unlock the system
 */
typedef void(FifoLockFn)(bool lock);

/**
 * @brief FIFO object.
 */
typedef struct {
    uint32_t level;     ///< FIFO utilization level
    uint32_t len;       ///< Number of slots
    uint32_t esize;     ///< Element size
    uint32_t read;      ///< Next item to read
    uint32_t write;     ///< Next item to write
    FifoLockFn *lockFn; ///< Locking function
    uint8_t *data;      ///< Pointer to the data pool
} Fifo;

#define FIFO_POOL(name, len, esize) uint8_t (name)[(len) * (esize)];

/**
 * Initialize the FIFO.
 *
 * @param f pointer to an uninitialized FIFO object
 * @param len number of slots
 * @param esize size of an element
 * @param data pointer to the data pool
 * @param lockFn pointer to the locking function
 */
void fifo_init(Fifo *f, uint32_t len, uint32_t esize, uint8_t *data, FifoLockFn lockFn);

/**
 * Push a new item into the FIFO.
 * (Thread-safe)
 *
 * @param f pointer to a FIFO object
 * @param item pointer to an item to push
 *
 * @return push successful
 */
bool fifo_push(Fifo *f, void const *item);

/**
 * Get the FIFO utilization level.
 *
 * @param pointer to a FIFO object
 *
 * @return utilization level
 */
uint32_t fifo_get_level(const Fifo *f);

/**
 * Pop the front item from the FIFO.
 * (Thread-safe)
 *
 * @param f pointer to a FIFO object
 * @param item pointer to a block where the item is going to be pushed
 *
 * @return pop successful
 */
bool fifo_pop(Fifo *f, void *item);

/** Clear the FIFO.
 * (Thread-safe)
 *
 * @param f pointer to a FIFO object
 */
void fifo_clear(Fifo *f);

#ifdef __cplusplus
}
#endif

#endif /* OSLESS_FIFO */
