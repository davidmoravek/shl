all:
	$(MAKE) -C src all
	mv src/shell build/shell
