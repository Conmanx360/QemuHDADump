CFLAGS = -O2 -Wall

targets = QemuHDADump ExtractHDADump 

.PHONY: clean all
all : $(targets)
clean:
	rm -f  $(targets)

QemuHDADump: QemuHDADump.c
	gcc $@.c -o $@  $(CFLAGS)

ExtractHDADump: ExtractHDADump.c
	gcc $@.c -o $@  $(CFLAGS)
