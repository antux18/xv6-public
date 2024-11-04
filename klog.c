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
#include "syslog.h"

#define NEVENTS 16

struct kernel_log {
    struct logev buf[NEVENTS];
    uint r, w;
    struct spinlock lock;
} klog;

int klogread(struct inode *ip, char *dst, int n) {
    acquire(&klog.lock);

    struct logev* evts = (struct logev*) dst;

    while (klog.r == klog.w) {
        sleep(&klog.r, &klog.lock);
    }

    int i;
    for (i = 0 ; i < (n/sizeof(struct logev)) && (klog.r != klog.w) ; i++) {
        evts[i] = klog.buf[klog.r % NEVENTS];
        klog.r++;
    }

    release(&klog.lock);

    if (n % sizeof(struct logev) != 0) {
        i++;
    }
    return i*sizeof(struct logev);
}

int klogwrite(struct inode *ip, char *buf, int n) {
    acquire(&klog.lock);

    struct logev* evts = (struct logev*) buf;

    if (klog.w >= klog.r + NEVENTS) {
        klog.r++;
    }

    int i;
    for (i = 0 ; i < (n/sizeof(struct logev)) ; i++) {
        klog.buf[klog.w % NEVENTS] = evts[i];
        klog.w++;
    }

    wakeup(&klog.r);
    release(&klog.lock);

    if (n % sizeof(struct logev) != 0) {
        i++;
    }
    return i*sizeof(struct logev);
}

void kloginit() {
    devsw[KLOG].write = klogwrite;
    devsw[KLOG].read = klogread;
}