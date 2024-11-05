/*
 * Programme débogueur minimaliste
 *
 * Utiliser avec :
 * 	db testdb a b c
 * testdb est exécuté sous le contrôle du débogueur et avec
 * les arguments "a", "b" et "c".
 *
 * Utiliser testdb.asm (généré automatiquement via le Makefile) pour
 * connaître les adresses dans le code.
 *
 * Le débogueur se limite à quelques commandes de base (cf. fonction "help").
 *
 * Exemple d'utilisation (cf. adresses dans testdb.asm) :
 * 	$ db testdb a b
 * 	db> m 20				# juste avant 1er printf
 *	Address 20 -> 57BE34FF (little endian)	# FF, puis 34, puis BE, etc.
 * 	db> b 20
 * 	breakpoint set at 0x20
 * 	db> r					# affiche les registres
 * 	...
 * 	eip = 0
 * 	db> s					# instruction à l'adr 0 (lea)
 * 	single step: eip=0x4
 *	db> s					# instruction à l'adr 4 (and)
 *	single step: eip=0x7
 * 	db> c					# juste avant 1er printf
 * 	Hit breakpoint at address 20
 * 	db> c
 * 	argv[1]=a				# printf argv[1] dans testdb
 * 	Hit breakpoint at address 20
 * 	db> q					# quit
 * 	$
 *
 */

#include "types.h"
#include "stat.h"
#include "user.h"
#include "ptrace.h"

#define	MAXBUF 512

// Commandes saisies par l'utilisateur
#define	CMD_REG		'r'
#define	CMD_SETBREAK	'b'
#define	CMD_DELBREAK	'd'
#define	CMD_STEP	's'
#define	CMD_CONT	'c'
#define	CMD_MEMORY	'm'
#define	CMD_QUIT	'q'

#define	INSTR_INT3	0xcc		// opcode de l'instruction "int3"

void
help(void)
{
  printf(2, "r      print registers\n");
  printf(2, "b addr define breakpoint at addr (hex)\n");
  printf(2, "d      delete breakpoint\n");
  printf(2, "s      single step\n");
  printf(2, "c      continue\n");
  printf(2, "m addr show memory at addr (hex)\n");
  printf(2, "q      quit\n");
}

char *
skip_spaces(char *s)
{
  while (*s == ' ' || *s == '\t' || *s == '\n')
    s++;
  return s;
}

// retourne la commande saisie par l'utilisateur
// et, selon le cas, place l'adresse indiquée (pour b et m)
// dans l'argument "addr"
int
getcmd(uint *addr)
{
  char buf[MAXBUF], *p;
  int cmd;

  cmd = 0;
  *addr = 0;

  do
  {
    printf(1, "db> ");
    memset(buf, 0, MAXBUF);
    gets(buf, MAXBUF-1);
    if(buf[0]==0){			// EOF
      cmd = CMD_QUIT;
    } else {
      p = skip_spaces (buf);
      switch (*p){
	// ligne vide : rien à faire
	case '\0':
	  break;

	// commandes sans argument
	case CMD_QUIT:
	case CMD_REG:
	case CMD_CONT:
	case CMD_STEP:
	case CMD_DELBREAK:
	  cmd = *p;
	  break;

	// commandes avec un argument (une adresse en hexa)
	case CMD_SETBREAK:
	case CMD_MEMORY:
	  cmd = *p;
	  p = skip_spaces(p+1);
	  while(*p != 0 && *p != ' ' && *p != '\t' && *p != '\n'){
	    if(*p >= '0' && *p <= '9')
	      *addr = *addr * 0x10 + (*p - '0');
	    else if(*p >= 'a' && *p <= 'f')
	      *addr = *addr * 0x10 + (*p - 'a');
	    else if(*p >= 'A' && *p <= 'F')
	      *addr = *addr * 0x10 + (*p - 'A');
	    else {
	      help();
	      cmd = 0;
	    }
	    p++;
	  }
	  break;

	default:
	  help();
      }
    }
  }
  while (cmd == 0);

  // printf(1,"cmd=%c, *addr=%x\n", cmd, *addr);

  return cmd;
}

// fonction unique pour tester le retour de tous les appels à ptrace
void
myptrace(int req, int pid, uint addr, uint *val)
{
  if(ptrace(req, pid, addr, val) == -1){
    // les PTRACE_* sont définis comme des codes ASCII, donc affichables
    printf(2, "cannot ptrace '%c'\n", req);
    exit();
  }
}

// Attendre que le fils soit prêt à recevoir les ordres du père
void
waitchild(int pid, int bpinmemory, uint bpaddr, uint bpval)
{
  regs_t regs;			// tableau de registres

  if(bpinmemory){
    // avant toute autre chose, supprimer le breakpoint dans la
    // mémoire du processus fils
    myptrace(PTRACE_WRITE, pid, bpaddr, &bpval) ;

    // avons-nous atteint ce breakpoint ?
    myptrace(PTRACE_GETREG, pid, 0, regs);
    if(regs[REG_EIP] == bpaddr + 1){
      regs[REG_EIP] -= 1;
      myptrace(PTRACE_SETREG, pid, 0, regs);
      printf(1, "Hit breakpoint at address %x\n", bpaddr);
    }
  } else {
    // attente du fils (pas d'utilisation des registres ici,
    // on ne fait qu'attendre)
    myptrace(PTRACE_GETREG, pid, 0, regs);
  }
}

int
main(int argc, char *argv[])
{
  uint arg;
  int cmd, pid;
  uint val;			// valeur à lire ou écrire en mémoire
  regs_t regs ;			// tous les registres
  int bpisset;			// un breakpoint est défini
  int bpinmemory;		// le breakpoint est écrit en mémoire
  uint bpaddr;			// adresse du breakpoint (un seul à la fois)
  uint bpval;			// ancienne valeur en mémoire à l'adresse

  if(argc <= 1){
    printf(2, "usage: %s prog args...\n", argv[0]);
    exit();
  }

  switch(pid = fork())
  {
    case -1:
      printf(2, "cannot fork\n");
      exit();

    case 0:
      // seul le processus fils doit exécuter "PTRACE_TRACE_ME"
      myptrace(PTRACE_TRACE_ME, 0, 0, 0);
      // le processus doit se mettre en attente avant la première
      // instruction du nouveau programme (i.e. à la fin de exec)
      exec(argv[1], argv+1);
      printf(2, "cannot exec %s\n", argv[1]);
      exit();

    default:
      break;
  }

  sleep(2);			// hack: laisser au fils le temps de démarrer

  bpisset = 0;
  bpinmemory = 0;
  bpaddr = bpval = 0;		// faire taire gcc et ses warnings

  while(waitchild(pid, bpinmemory, bpaddr, bpval),
	  (cmd = getcmd(&arg)) != CMD_QUIT){

    bpinmemory = 0;		// waitchild a supprimé l'instruction "int3"

    switch(cmd){
      case CMD_MEMORY:
	myptrace (PTRACE_READ, pid, arg, &val);
	// la valeur est à lire de droite à gauche (x86 = little endian)
	printf(1, "Address %x -> %x (little endian)\n", arg, val);
	break;

      case CMD_REG:
	myptrace (PTRACE_GETREG, pid, 0, regs) ;
	printf(1, "eax = %x\n", regs [REG_EAX]);
	printf(1, "ebx = %x\n", regs [REG_EBX]);
	printf(1, "ecx = %x\n", regs [REG_ECX]);
	printf(1, "edx = %x\n", regs [REG_EDX]);
	printf(1, "edi = %x\n", regs [REG_EDI]);
	printf(1, "esi = %x\n", regs [REG_ESI]);
	printf(1, "ebp = %x\n", regs [REG_EBP]);
	printf(1, "esp = %x\n", regs [REG_ESP]);
	printf(1, "eip = %x\n", regs [REG_EIP]);
	printf(1, "eflags = %x\n", regs [REG_EFLAGS]);
	break;

      case CMD_STEP:
	myptrace(PTRACE_STEP, pid, 0, 0) ;
	myptrace(PTRACE_GETREG, pid, 0, regs);
	printf(1, "single step: eip=0x%x\n", regs[REG_EIP]);
	break;

      case CMD_CONT:
	if(bpisset){
	  uint b;
	  // sauter le breakpoint si on est devant
	  myptrace(PTRACE_GETREG, pid, 0, regs);
	  if (regs[REG_EIP] == bpaddr){		// on est au breakpoint
	      myptrace(PTRACE_STEP, pid, 0, 0);	// sauter le breakpoint
	  }
	  myptrace(PTRACE_READ, pid, bpaddr, &bpval);	// ancienne valeur
	  // puisque l'instruction "int3" occupe un seul octet et que
	  // les accès à la mémoire du fils via ptrace sont sur 4 octets,
	  // nous devons uniquement modifier l'octet de poids faible.
	  b = (bpval & 0xffffff00) | INSTR_INT3;
	  myptrace(PTRACE_WRITE, pid, bpaddr, &b);
	  bpinmemory = 1;
	}
	myptrace (PTRACE_CONT, pid, 0, 0) ;
	break;

      case CMD_SETBREAK:
	// mémoriser l'adresse du breakpoint
	bpisset = 1;
	bpaddr = arg;
	printf(1, "breakpoint set at 0x%x\n", bpaddr);
	break;

      case CMD_DELBREAK:
	// supprimer l'adresse du breakpoint
	if(bpisset){
	  printf(1, "breakpoint deleted at 0x%x\n", bpaddr);
	} else {
	  printf(1, "no breakpoint to delete\n");
	}
	bpisset = 0;
	break;
    }
  }

  exit();
}
