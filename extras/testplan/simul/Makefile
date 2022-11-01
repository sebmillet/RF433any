# This Makefile is to be symbolic-linked inside subfolders.
# It is generic enough that, to the asumption the subfolder contains one .ino
# file and only one, it'll work as expected.

source = $(wildcard *.ino)
target = build/$(source).with_bootloader.hex
opt = -t 3

ALL: $(target)

$(target): $(source)
ifdef color
	./am2 $(opt) $<
else
	GCC_COLORS="" ./am2 --no-color $(opt) $<
endif

clean:
	rm -rf build

mrproper:
	rm -rf build out
