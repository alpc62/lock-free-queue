ALL_TARGETS := all check clean

.PHONY: $(ALL_TARGETS)

all:
	@echo "Only header, you can include it directly; or try -- make check"

check:
	make -C test check

clean:
	-make -C test clean
