.PHONY: error export

error:
	@echo "only 'make export' is allowed"
	@exit 1

export: $(BUILDS)
	mkdir -p $(EXPORT_DIR)
	$(MAKE) -r -C lib/posix export EXPORT_DIR=$(EXPORT_DIR)
