// TODO: license header

#include "posix/mntent.h"
#include <stdlib.h>

struct mntent *getmntent(FILE *f)
{
	(void) f;
	fprintf(stderr, "getmntent() not implemented");
	abort();
}
