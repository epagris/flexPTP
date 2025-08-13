#ifndef FLEXPTP_MSG_BUF
#define FLEXPTP_MSG_BUF

#include <stdbool.h>
#include <stdint.h>

#include "ptp_types.h"

#define MSGBUF_TTL_DONT_AGE (0xFFFFFFFF) ///< Do not age messages

#define MSGBUF_TAG_OVERWRITE (0x80000000) ///< Overwrite if a message exists with the same tag

/**
 * @brief PTP message buffer entry.
 */
typedef struct {
    bool allocated;    ///< This block has been allocated
    bool committed;    ///< This block has been committed
    bool sent;         ///< This block has been sent
    uint32_t tag;      ///< Block tag
    uint32_t uid;      ///< Unique ID, a sequence number
    uint32_t ttl;      ///< Time-to-Live in ticks
    RawPtpMessage msg; ///< The contained PTP message
} PtpMsgBufBlock;

/**
 * @brief Error enumeration for the message buffer.
 */
typedef enum {
    MSGB_ERR_NONE = 0, ///< No error
    MSGB_ERR_FULL,     ///< The buffer is full, cannot allocate a slot
    MSGB_ERR_EXISTS,   ///< An allocated message has already been stored under the same tag
    MSGB_ERR_MISC,     ///< Miscellaneous allocation error
} PtpMsgBufError;

/**
 * @brief PTP message buffer.
 */
typedef struct {
    uint32_t n;             ///< Number of blocks
    uint32_t used;          ///< Number of used blocks
    uint32_t lastUId;       ///< Last UID
    uint32_t error;         ///< Last error
    PtpMsgBufBlock *blocks; ///< Block pool
} PtpMsgBuf;

/**
 * Initialize PTP message buffer.
 *
 * @param buf pointer to an empty (non-initialized) PtpMsgBuf object
 * @param pool pointer to an allocated pool for n pieces of PtpMsgBufEntry
 * @param n number of supported elements
 */
void msgb_init(PtpMsgBuf *buf, PtpMsgBufBlock *pool, uint32_t n);

/**
 * Allocate a block.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param tag unique tag
 * @param ttl Time-to-Live in ticks
 *
 * @return pointer to a RawPtpMessage object or NULL if there's a block already
 * allocated with the current tag or the buffer is full
 */
RawPtpMessage *msgb_alloc(PtpMsgBuf *buf, uint32_t tag, uint32_t ttl);

/**
 * Commit a previous allocation, make the message available
 * for later pulling.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param msg pointer to the allocated message
 */
void msgb_commit(PtpMsgBuf *buf, RawPtpMessage *msg);

/**
 * Release an allocated message block.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param msg pointer to the allocated message
 */
void msgb_free(PtpMsgBuf *buf, RawPtpMessage *msg);

/**
 * Get oldest message from the buffer. (i.e. sequential reading)
 *
 * @param buf pointer to the PtpMsgBuf object
 *
 * @return pointer to the oldest message or NULL if the buffer is empty
 */
RawPtpMessage *msgb_get_oldest(PtpMsgBuf *buf);

/**
 * Get message by its UID.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param uid uid of the message sought
 *
 * @return message with the UID or NULL if not found
 */
RawPtpMessage *msgb_get_by_uid(PtpMsgBuf *buf, uint32_t uid);

/**
 * Set sent flag for a message.
 *
 * @param buf pointer to PtpMsgBuf object
 * @param msg pointer to an allocated message
 */
void msgb_set_sent(PtpMsgBuf *buf, RawPtpMessage *msg);

/**
 * Get the UID of an allocated message.
 *
 * @param buf pointer to PtpMsgBuf object
 * @param msg pointer to an allocated message
 *
 * @return message UID
 */
uint32_t msgb_get_uid(const PtpMsgBuf *buf, const RawPtpMessage *msg);

/**
 * Get sent message by tag.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param tag tag of the message sought
 *
 * @return message with the tag or NULL if not found
 */
RawPtpMessage *msgb_get_sent_by_tag(PtpMsgBuf *buf, uint32_t tag);

/**
 * Tick the storage.
 *
 * @param buf pointer to the PtpMsgBuf object
 */
void msgb_tick(PtpMsgBuf *buf);

/**
 * Report the message buffer state.
 *
 * @param buf pointer to the PtpMsgBuf object
 */
void msgb_report(PtpMsgBuf *buf);

/**
 * Read and clear last error.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @return error code
 */
uint32_t msgb_get_error(PtpMsgBuf * buf);

#endif /* FLEXPTP_MSG_BUF */
