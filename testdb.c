/*
 * Programme pour accompagner le développement de ptrace.
 * Attention : ce programme n'est utile que lorsqu'il est exécuté
 * par le débogueur db :
 * 	db testdb a b c
 *
 * Ce programme ne fait que faire un affichage, puis il génère
 * artificiellement génère une exception "breakpoint" (instruction
 * "int3") qui doit provoquer la suspension de ce processus et
 * la synchronisation avec la primitive ptrace utilisée par le débogueur).
 */

#include "types.h"
#include "stat.h"
#include "user.h"

static inline void
int3(void)
{
  asm volatile("int3");
}

int
main(int argc, char *argv[])
{
  int i;

  // quelques instructions pour faire semblant de faire quelque chose
  for (i = 1 ; i < argc ; i++)
    printf(1, "argv[%d]=%s\n", i, argv [i]);

  // provoque une entrée dans le débogueur
  int3();

  // on fait toujours semblant, mais dans l'autre sens...
  for (i = argc - 1 ; i > 0 ; i--)
    printf(1, "argv[%d]=%s\n", i, argv [i]);

  // provoque une nouvelle entrée dans le débogueur
  int3();

  exit();
}
