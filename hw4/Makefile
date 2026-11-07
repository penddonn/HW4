.PHONY: all release debug p1 p2 clean
all: release
release: p1 p2
p1:
	$(MAKE) -C p1 release
p2:
	$(MAKE) -C p2 release
debug:
	$(MAKE) -C p1 debug
	$(MAKE) -C p2 debug
clean:
	$(MAKE) -C p1 clean
	$(MAKE) -C p2 clean
