#include "posix/stdio.h"
#include <stdlib.h>

void __fpurge(FILE *fp)
{
	fprintf(stderr, "__fpurge not implemented");
	abort();
}
