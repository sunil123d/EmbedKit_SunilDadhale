# EmbedKit_YourName

**Name:** Sunil Dadhale

Embedded C utility library - Embed Square Solutions fresher assignment.

## Build and run

```
gcc -Wall -std=c99 ringbuf.c -o ringbuf
./ringbuf
```

Compiles with zero warnings and zero errors. Uses standard C only (`<stdio.h>`, `<stdint.h>`).

## Modules

| File | Description |
|------|-------------|
| `ringbuf.c` | 8-byte `uint8_t` circular FIFO with safe write/read (error codes on full/empty), count/full/empty queries, and `& (BUFFER_SIZE - 1)` wrap-around instead of `%`. |
