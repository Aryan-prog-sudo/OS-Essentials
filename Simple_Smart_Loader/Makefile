all:
	@echo "--- Building Loader Library ---"
	$(MAKE) -C loader
	@echo "--- Building Test Executable ---"
	$(MAKE) -C test
	@echo "--- Building Launcher ---"
	$(MAKE) -C launcher
	@echo "--- Build Complete ---"

clean:
	@echo "--- Cleaning All ---"
	$(MAKE) -C loader clean
	$(MAKE) -C test clean
	$(MAKE) -C launcher clean
	rm -f bin/*
	@echo "--- Clean Complete ---"