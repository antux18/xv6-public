#include "types.h"
#include "user.h"

void
testpid(int argc)
{
  if (argc == 1)
    printf(1, "pid = %d, ppid = %d\n", getpid(), getppid());
  else
    // les fonctions getpid2 et getppid2 sont utiles pour la question 7
    printf(1, "page2 : pid = %d, ppid = %d\n", getpid2(), getppid2());
}

int
main(int argc, char *argv[])
{
  if(argc > 2){
    printf(2, "usage: %s [nimportequoi]\n", argv[0]);
    exit();
  }

  testpid (argc);

  printf (1, "test avec un processus fils adopte par le processus 1\n");
  switch (fork())
  {
    case -1:
      printf(2, "cannot fork\n");
      exit();
    case 0:
      sleep (10);
      printf(1, "\n");		// passer le prompt du shell libéré par le père
      testpid(argc);
      exit();
    default:
      break;
  }
  exit();
}
