#include "types.h"
#include "user.h"
#include "fcntl.h"

#define MAX 512

#define MIN(a,b)	((a)<(b)?(a):(b))

int
main(int argc, char *argv[])
{
  int fd, o, n, r ;
  int ref;
  char buf[MAX];

  if(argc != 4){
    printf(2, "usage: testlseek file o n\n");
    exit();
  }

  fd = open (argv [1], O_RDONLY) ;
  if(fd == -1) {
    printf(2, "testlseek: cannot open %s\n", argv[1]);
    exit();
  }
  o = atoi (argv [2]) ;
  n = atoi (argv [3]) ;
  ref = (o >= 0) ? SEEK_SET : SEEK_END ;
  if (lseek (fd, o, ref) < 0) {
    printf(2, "testlseek: cannot lseek %d from %s\n", o,
	(ref == SEEK_SET) ? "SEEK_SET" : "SEEK_END");
    exit();
  }
  printf(2, "testlseek: %s fd=%d o=%d n=%d ref=%d\n", argv[1],fd,o,n,ref) ;
  while (n > 0 && (r = read (fd, buf, MIN (n, sizeof buf))) > 0)
  {
    write (1, buf, r) ;
    n -= r ;
  }
  if (close (fd) == -1) {
    printf(2, "testlseek: cannot close %s\n", argv[1]);
    exit();
  }
  exit();
}
