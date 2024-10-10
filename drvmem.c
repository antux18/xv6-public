// TP2

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define DEV_NULL 0
#define DEV_ZERO 1
#define DEV_MEM 2

int drvmemread(struct inode *ip, char *dst, uint off, int n) {
    switch (ip->minor) {
        case DEV_NULL:
            return 0;
            break;
        case DEV_ZERO:
            memset(dst, 0, n);
            return n;
            break;
        case DEV_MEM:
            if (off < EXTMEM || off > PHYSTOP || off+n < EXTMEM || off+n > PHYSTOP) {
                return -1;
            }
            else {
                // memmove(dst);
            }
            return 0;
        default:
            return -1;
    }
}

int drvmemwrite(struct inode *ip, char *buf, uint off, int n) {
    switch (ip->minor) {
        case DEV_NULL:
            return n;
            break;
        case DEV_ZERO:
            return n;
            break;
        case DEV_MEM:
            return 0;
        default:
            return -1;
    }
}

void drvmeminit() {
    devsw[DRVMEM].write = drvmemwrite;
    devsw[DRVMEM].read = drvmemread;
}