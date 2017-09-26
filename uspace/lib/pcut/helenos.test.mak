# XXX: Edited manually because this requires upstream changes.

# Re-generate with CMake

# abort
$(PCUT_TEST_PREFIX)abort$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/abort.o

# asserts
$(PCUT_TEST_PREFIX)asserts$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/asserts.o

# beforeafter
$(PCUT_TEST_PREFIX)beforeafter$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/beforeafter.o

# errno
$(PCUT_TEST_PREFIX)errno$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/errno.o

# inithook
$(PCUT_TEST_PREFIX)inithook$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/inithook.o

# manytests
$(PCUT_TEST_PREFIX)manytests$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/manytests.o

# multisuite
$(PCUT_TEST_PREFIX)multisuite$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/suite_all.o $(BUILD_DIR)/tests/suite1.o $(BUILD_DIR)/tests/suite2.o $(BUILD_DIR)/tests/tested.o

# preinithook
$(PCUT_TEST_PREFIX)preinithook$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/inithook.o

# printing
$(PCUT_TEST_PREFIX)printing$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/printing.o

# simple
$(PCUT_TEST_PREFIX)simple$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/simple.o $(BUILD_DIR)/tests/tested.o

# skip
$(PCUT_TEST_PREFIX)skip$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/skip.o

# suites
$(PCUT_TEST_PREFIX)suites$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/suites.o $(BUILD_DIR)/tests/tested.o

# teardownaborts
$(PCUT_TEST_PREFIX)teardownaborts$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/teardownaborts.o

# teardown
$(PCUT_TEST_PREFIX)teardown$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/teardown.o $(BUILD_DIR)/tests/tested.o

# testlist
$(PCUT_TEST_PREFIX)testlist$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/testlist.o

# timeout
$(PCUT_TEST_PREFIX)timeout$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/timeout.o

# xmlreport
$(PCUT_TEST_PREFIX)xmlreport$(PCUT_TEST_SUFFIX): $(BUILD_DIR)/tests/xmlreport.o $(BUILD_DIR)/tests/tested.o

