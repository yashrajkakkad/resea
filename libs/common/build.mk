name := common
objs-y += kasan.o string.o vprintf.o ubsan.o bitmap.o
subdirs-y += arch/$(ARCH)
