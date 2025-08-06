#include "msg_buf.h"

#include "minmax.h"

void msgb_init(PtpMsgBuf *buf, PtpMsgBufBlock *pool, uint32_t n) {
    buf->blocks = pool;
    buf->lastUId = 0;
    buf->n = n;
    buf->used = 0;
}

/**
 * Retrieve entry by tag.
 *
 * @param buf pointer to PtpMsgBuf object
 * @param tag block tag
 *
 * @return corresponding allocated entry if found OR NULL
 */
static PtpMsgBufBlock *msgb_get_entry_by_tag(PtpMsgBuf *buf, uint32_t tag) {
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock *block = buf->blocks + i;
        if ((block->allocated) && (block->tag == tag)) {
            return block;
        }
    }
    return NULL;
}

RawPtpMessage *msgb_alloc(PtpMsgBuf *buf, uint32_t tag, uint32_t ttl) {
    if (buf->used == buf->n) { // the buffer is full
        return NULL;
    }

    // let's see if the tag is unique or generate a unique tag
    if (tag == 0) {
        do {
            tag = rand() & (~((uint32_t)0));
        } while (msgb_get_entry_by_tag(buf, tag) != NULL);
    } else {
        if (msgb_get_entry_by_tag(buf, tag) != NULL) {
            return NULL;
        }
    }

    // allocate a new block
    PtpMsgBufBlock *block = NULL;
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock *blockIter = buf->blocks + i;
        if (!blockIter->allocated) {
            block = blockIter;
            break;
        }
    }

    // avert some impossible errors
    if (block == NULL) {
        return NULL;
    }

    // indicate that this block is now allocated but has not been committed yet
    block->allocated = true;
    block->committed = false;
    block->sent = false;

    // set UID and user-defined options
    block->uid = ++buf->lastUId;
    block->tag = tag;
    block->ttl = ttl;

    // a new block has been allocated
    buf->used++;

    // return the allocated message area
    return &(block->msg);
}

#define RAW_MSG_TO_BLOCK(rmsg) ((PtpMsgBufBlock *)(((uint8_t *)(rmsg)) - (sizeof(PtpMsgBufBlock) - sizeof(RawPtpMessage)))) ///< Get block address by message address

void msgb_commit(PtpMsgBuf *buf, RawPtpMessage *msg) {
    PtpMsgBufBlock *block = RAW_MSG_TO_BLOCK(msg);
    block->committed = true;
}

/**
 * Release an allocated block.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param block pointer to a previously allocated block
 */
static void msgb_free_block(PtpMsgBuf *buf, PtpMsgBufBlock *block) {
    block->allocated = 0;
    block->committed = 0;
    buf->used--;
}

void msgb_free(PtpMsgBuf *buf, RawPtpMessage *msg) {
    PtpMsgBufBlock *block = RAW_MSG_TO_BLOCK(msg);
    msgb_free_block(buf, block);
}

/**
 * Get oldest block from the buffer. (i.e. sequential reading)
 *
 * @param buf pointer to the PtpMsgBuf object
 *
 * @return pointer to the oldest block or NULL if the buffer is empty
 */
static PtpMsgBufBlock *msgb_get_oldest_block(PtpMsgBuf *buf) {
    if (buf->used == 0) {
        return NULL;
    }

    PtpMsgBufBlock *oldestBlock = NULL;
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock *block = buf->blocks + i;
        if (block->allocated && block->committed &&
            ((oldestBlock == NULL) || (block->uid < oldestBlock->uid))) {
            oldestBlock = block;
        }
    }
    return oldestBlock;
}

RawPtpMessage *msgb_get_oldest(PtpMsgBuf *buf) {
    PtpMsgBufBlock *block = msgb_get_oldest_block(buf);
    if (block == NULL) {
        return NULL;
    } else {
        return &(block->msg);
    }
}

/**
 * Get block by its UID.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param uid uid of the message sought
 *
 * @return message block with the UID or NULL if not found
 */
static PtpMsgBufBlock *msgb_get_block_by_uid(PtpMsgBuf *buf, uint32_t uid) {
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock *block = buf->blocks + i;
        if (block->allocated && block->committed && (block->uid == uid)) {
            return block;
        }
    }
    return NULL;
}

RawPtpMessage *msgb_get_by_uid(PtpMsgBuf *buf, uint32_t uid) {
    PtpMsgBufBlock *block = msgb_get_block_by_uid(buf, uid);
    if (block == NULL) {
        return NULL;
    } else {
        return &(block->msg);
    }
}

/**
 * Get sent block by tag.
 *
 * @param buf pointer to the PtpMsgBuf object
 * @param uid uid of the message sought
 *
 * @return sent message block with the tag or NULL if not found
 */
static PtpMsgBufBlock *msgb_get_sent_block_by_tag(PtpMsgBuf *buf, uint32_t tag) {
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock *block = buf->blocks + i;
        if (block->allocated && block->committed && block->sent && (block->tag == tag)) {
            return block;
        }
    }
    return NULL;
}

RawPtpMessage *msgb_get_sent_by_tag(PtpMsgBuf *buf, uint32_t tag) {
    PtpMsgBufBlock *block = msgb_get_sent_block_by_tag(buf, tag);
    if (block == NULL) {
        return NULL;
    } else {
        return &(block->msg);
    }
}

/**
 * Set sent flag in a block.
 *
 * @param buf pointer to PtpMsgBuf object
 * @param block pointer to a block
 */
static void msgb_set_block_sent(PtpMsgBuf *buf, PtpMsgBufBlock *block) {
    block->sent = true;
}

void msgb_set_sent(PtpMsgBuf *buf, RawPtpMessage *msg) {
    PtpMsgBufBlock *block = RAW_MSG_TO_BLOCK(msg);
    msgb_set_block_sent(buf, block);
}

void msgb_tick(PtpMsgBuf *buf) {
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock *block = buf->blocks + i;
        if (block->allocated && (block->ttl != MSGBUF_TTL_DONT_AGE)) {
            block->ttl = (block->ttl > 0) ? (block->ttl - 1) : block->ttl;
            if (block->ttl == 0) {
                msgb_free_block(buf, block);
            }
        }
    }
}

void msgb_report(PtpMsgBuf *buf) {
    MSG("----------------------------------\n");
    for (uint32_t i = 0; i < buf->n; i++) {
        PtpMsgBufBlock * block = buf->blocks + i;
        if (block->allocated) {
            MSG("[% 3u] A % 8X % 8X % 4u %c %s\n", i, block->tag, block->uid, block->ttl, block->committed ? 'C' : ' ', block->sent ? 'S' : ' ');
        } else {
            MSG("[% 3u] F -------------------\n", i);
        }
    }
    MSG("----------------------------------\n");
}