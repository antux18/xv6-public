#define PTRACE_TRACE_ME	'M'	// used by child process only
#define PTRACE_CONT	'C'	// continue child until next breakpoint or exit
#define PTRACE_STEP	'T'	// single step child
#define PTRACE_GETREG	'G'	// get registers from child process
#define PTRACE_SETREG	'S'	// set child process registers
#define PTRACE_READ	'R'	// read memory from child process
#define PTRACE_WRITE	'W'	// write into child process memory

// Indices in the reg array
#define	REG_EAX		0
#define	REG_EBX		1
#define	REG_ECX		2
#define	REG_EDX		3
#define	REG_EDI		4
#define	REG_ESI		5
#define	REG_EBP		6
#define	REG_ESP		7
#define	REG_EIP		8
#define	REG_EFLAGS	9
#define	NREGS		10

typedef uint regs_t [NREGS];	// type regs_t = array of NREGS uint
