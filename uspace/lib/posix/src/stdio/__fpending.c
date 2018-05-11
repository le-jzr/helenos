
// TODO

#include "posix/stdio.h"
#include <stdlib.h>

/* Return the number of pending (aka buffered, unflushed)
   bytes on the stream, FP, that is open for writing.  */
size_t
__fpending (FILE *fp)
{
	fprintf(stderr, "__fpending not implemented");
	abort();
}
