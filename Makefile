all:
	$(MAKE) -C plugin all
	$(MAKE) -C libs all

clean:
	$(MAKE) -C plugin clean
	$(MAKE) -C libs clean
