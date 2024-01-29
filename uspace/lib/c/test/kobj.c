#include <errno.h>
#include <pcut/pcut.h>
#include <abi/mm/as.h>
#include <libarch/config.h>
#include <kobj.h>
#include <as.h>

PCUT_INIT;

PCUT_TEST_SUITE(kobj);

static int read(void *vaddr)
{
	return *((volatile int *) vaddr);
}

static void write(void *vaddr, int val)
{
	*((volatile int *) vaddr) = val;
}

PCUT_TEST(kobj_sharedmem)
{
	size_t mem_size = 2 * PAGE_SIZE;

	/* Repeat the whole thing a couple thousand times to check for any leaks. */
	for (int i = 0; i < 10000; i++) {
		mem_t mem = sys_mem_create(mem_size, NULL);

		void *vaddr1 = AS_AREA_ANY;
		errno_t rc = sys_mem_map(mem, &vaddr1, 0, mem_size);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);

		void *vaddr2 = AS_AREA_ANY;
		rc = sys_mem_map(mem, &vaddr2, 0, mem_size);
		PCUT_ASSERT_ERRNO_VAL(EOK, rc);

		PCUT_ASSERT_FALSE(vaddr1 == vaddr2);

		PCUT_ASSERT_INT_EQUALS(0, read(vaddr1));
		write(vaddr2, 12345);
		PCUT_ASSERT_INT_EQUALS(12345, read(vaddr1));
		write(vaddr1, 54321);
		PCUT_ASSERT_INT_EQUALS(54321, read(vaddr2));

		void *vaddr3 = AS_AREA_ANY;
		rc = sys_mem_map(mem, &vaddr3, PAGE_SIZE, PAGE_SIZE);

		sys_kobj_put(mem);

		PCUT_ASSERT_INT_EQUALS(0, read(vaddr3));
		write(vaddr1 + PAGE_SIZE, 1);
		PCUT_ASSERT_INT_EQUALS(1, read(vaddr3));

		as_area_destroy(vaddr1);
		as_area_destroy(vaddr2);
		as_area_destroy(vaddr3);
	}
}

PCUT_EXPORT(kobj);
