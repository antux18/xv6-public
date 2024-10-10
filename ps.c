#include "types.h"
#include "fcntl.h"
#include "user.h"

#include "param.h"
#include "mmu.h"
#include "spinlock.h"
#include "proc.h"

struct
{
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

uint
hex2off(char *h)
{
  uint off;

  off=0;
  for(;*h != '\0'; h++)
  {
    int d;

    if (*h >= '0' && *h <= '9')
      d = *h - '0';
    else if (*h >= 'a' && *h <= 'f')
      d = *h - 'a' + 10;
    else if (*h >= 'A' && *h <= 'F')
      d = *h - 'A' + 10;
    else
    {
      printf(2, "illegal hex digit\n") ;
      exit () ;
    }
    off = (off << 4) + d;
  }
  return off;
}

char
procstate2str(enum procstate s)
{
  return s == SLEEPING ? 's' :
      (s == RUNNABLE ? 'r' :
      (s == RUNNING ? 'R' :
      (s == ZOMBIE ? 'z' :
	'?'))) ;
}

int
main(int argc, char *argv[])
{
  uint off;
  int i, fd;

  if(argc != 3){
    printf(2, "Usage: ps kmem proc-addr\n");
    exit();
  }

  off = hex2off (argv [2]) ;
  printf (1, "off=%x\n", off);
  if ((fd = open (argv[1], O_RDONLY)) == -1) {
    printf(2, "cannot open %d\n", argv[1]);
    exit();
  }
  if (lseek(fd, off, SEEK_SET) == -1) {
    printf(2, "cannot lseek to %d\n", off);
    exit();
  }
  i = read(fd, &ptable, sizeof ptable);
  printf (1, "read %d bytes\n", i) ;
  if (i == -1) {
    printf(2, "cannot read %d procs", NPROC);
    exit();
  }
  if (close(fd) == -1) {
    printf(2, "cannot close %s\n", argv[1]);
    exit();
  }

  unsigned char *p = (unsigned char *) &ptable.proc;
  for (i = 0 ; i < 80 ; i++)
  {
    if (i > 0 && i % 16 == 0)
	printf (1, "\n") ;
    printf (1, " %x%x", p[i] >> 4, p[i] & 0xf) ;
  }
  printf (1,"\n") ;

  for (i = 0 ; i < NPROC ; i++) {
    if (ptable.proc [i].state == UNUSED || ptable.proc [i].state == EMBRYO)
      continue;
    printf (1, "%d %c %s\n",
    		ptable.proc [i].pid,
    		procstate2str (ptable.proc [i].state),
		ptable.proc [i].name) ;
  }

  exit();
}
