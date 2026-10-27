/*
 * ringbuf.c - Ring Buffer (circular FIFO) for uint8_t data
 * Embed Kit: Embedded C Utility Library
 *
 * Build: gcc -Wall -std=c99 ringbuf.c -o ringbuf
 */

#include <stdio.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Configuration constants                                             */
/* ------------------------------------------------------------------ */
#define BUFFER_SIZE         8U                  /* MUST be a power of 2 */
#define BUFFER_INDEX_MASK   (BUFFER_SIZE - 1U)  /* 8 -> 0b00000111      */

#define RB_OK               0                   /* operation succeeded  */
#define RB_ERR_FULL         (-1)                /* write on full buffer */
#define RB_ERR_EMPTY        (-2)                /* read on empty buffer */

/* Demo data */
#define DEMO_BYTE_OVERFLOW  0x99U
#define DEMO_FIRST_BYTE     0x41U               /* 'A' */
#define DEMO_REFILL_BYTE    0x49U               /* continues after 0x48 */
#define DEMO_READ_COUNT     3U
#define DEMO_REFILL_COUNT   3U

/* ------------------------------------------------------------------ */
/* Types                                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t data[BUFFER_SIZE];  /* storage                              */
    uint8_t head;               /* index of the next write              */
    uint8_t tail;               /* index of the next read               */
    uint8_t count;              /* number of unread bytes currently held */
} RingBuffer;

/* ------------------------------------------------------------------ */
/* Ring buffer API                                                     */
/* ------------------------------------------------------------------ */

/* Put the buffer into the empty state. */
static void ringbuf_init(RingBuffer *rb)
{
    uint8_t i;

    for (i = 0U; i < BUFFER_SIZE; i++) {
        rb->data[i] = 0U;
    }
    rb->head  = 0U;
    rb->tail  = 0U;
    rb->count = 0U;
}

/* Number of unread bytes stored. */
static uint8_t ringbuf_count(const RingBuffer *rb)
{
    return rb->count;
}

/* Returns 1 if the buffer is full, else 0. */
static uint8_t ringbuf_is_full(const RingBuffer *rb)
{
    return (rb->count == BUFFER_SIZE) ? 1U : 0U;
}

/* Returns 1 if the buffer is empty, else 0. */
static uint8_t ringbuf_is_empty(const RingBuffer *rb)
{
    return (rb->count == 0U) ? 1U : 0U;
}

/*
 * Write one byte. Returns RB_OK, or RB_ERR_FULL if there is no room.
 * Unread data is never overwritten.
 */
static int8_t ringbuf_write(RingBuffer *rb, uint8_t byte)
{
    if (ringbuf_is_full(rb)) {
        return RB_ERR_FULL;
    }

    rb->data[rb->head] = byte;

    /*
     * Wrap-around using a bitwise AND instead of "% BUFFER_SIZE".
     *
     * Why it is faster: the % operator needs an integer division. Many
     * small MCUs (e.g. Cortex-M0, AVR, PIC) have no hardware divider, so
     * the compiler calls a slow software routine taking tens to hundreds
     * of cycles - bad inside an ISR. A bitwise AND is a single-cycle
     * instruction.
     *
     * Why it needs a power-of-2 size: for N = 2^k, (N - 1) is a mask of k
     * ones (8 -> 0b0111). ANDing with it keeps only the low k bits, which
     * is exactly "value mod N". For a size like 10, (10 - 1) = 0b1001 is
     * not a solid run of ones, so the AND gives wrong indices.
     */
    rb->head = (uint8_t)((rb->head + 1U) & BUFFER_INDEX_MASK);
    rb->count++;

    return RB_OK;
}

/*
 * Read one byte into *byte. Returns RB_OK, or RB_ERR_EMPTY if there is
 * nothing to read (*byte is left untouched in that case).
 */
static int8_t ringbuf_read(RingBuffer *rb, uint8_t *byte)
{
    if (ringbuf_is_empty(rb)) {
        return RB_ERR_EMPTY;
    }

    *byte = rb->data[rb->tail];
    rb->tail = (uint8_t)((rb->tail + 1U) & BUFFER_INDEX_MASK); /* see above */
    rb->count--;

    return RB_OK;
}

/* ------------------------------------------------------------------ */
/* Demo helpers (print one line per operation)                         */
/* ------------------------------------------------------------------ */
static void demo_write(RingBuffer *rb, uint8_t byte)
{
    int8_t result = ringbuf_write(rb, byte);

    if (result == RB_OK) {
        printf("[WRITE] 0x%02X -> OK (count=%u)%s\n",
               (unsigned)byte, (unsigned)ringbuf_count(rb),
               ringbuf_is_full(rb) ? " FULL" : "");
    } else {
        printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", (unsigned)byte);
    }
}

static void demo_read(RingBuffer *rb)
{
    uint8_t byte = 0U;
    int8_t  result = ringbuf_read(rb, &byte);

    if (result == RB_OK) {
        printf("[READ] -> 0x%02X (count=%u)%s\n",
               (unsigned)byte, (unsigned)ringbuf_count(rb),
               ringbuf_is_empty(rb) ? " EMPTY" : "");
    } else {
        printf("[READ] (empty) -> FAIL (buffer empty)\n");
    }
}

/* ------------------------------------------------------------------ */
/* Demonstration                                                       */
/* ------------------------------------------------------------------ */
int main(void)
{
    RingBuffer rb;
    uint8_t i;

    ringbuf_init(&rb);

    printf("--- Step 1: fill the buffer with %u bytes ---\n", (unsigned)BUFFER_SIZE);
    for (i = 0U; i < BUFFER_SIZE; i++) {
        demo_write(&rb, (uint8_t)(DEMO_FIRST_BYTE + i));
    }
    printf("Full=%s, count=%u\n\n",
           ringbuf_is_full(&rb) ? "YES" : "NO", (unsigned)ringbuf_count(&rb));

    printf("--- Step 2: write to a full buffer ---\n");
    demo_write(&rb, DEMO_BYTE_OVERFLOW);
    printf("\n");

    printf("--- Step 3: read %u bytes ---\n", (unsigned)DEMO_READ_COUNT);
    for (i = 0U; i < DEMO_READ_COUNT; i++) {
        demo_read(&rb);
    }
    printf("Count is now %u\n\n", (unsigned)ringbuf_count(&rb));

    printf("--- Step 4: write %u new bytes (slots reused) ---\n", (unsigned)DEMO_REFILL_COUNT);
    for (i = 0U; i < DEMO_REFILL_COUNT; i++) {
        demo_write(&rb, (uint8_t)(DEMO_REFILL_BYTE + i));
        
    }
    printf("Count is back to %u\n\n", (unsigned)ringbuf_count(&rb));

    printf("--- Step 5: read all remaining bytes ---\n");
    while (!ringbuf_is_empty(&rb)) {
        demo_read(&rb);
    }
    printf("Empty=%s\n\n", ringbuf_is_empty(&rb) ? "YES" : "NO");

    printf("--- Step 6: read from an empty buffer ---\n");
    demo_read(&rb);

    return 0;
}