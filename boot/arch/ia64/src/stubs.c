// GNU ld doesn't support --gc-sections for IA64.

void *malloc(unsigned long);
void *malloc(unsigned long sz)
{
	return 0;
}

