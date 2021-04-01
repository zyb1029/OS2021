#include <common.h>
#include <stdatomic.h>

int cpu_current() {
	return getpid();	
}

int atomic_xchg(int *addr, int newval) {
	return atomic_exchange((int *)addr, newval);	
}
