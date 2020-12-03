name := common
objs-y += string.o vprintf.o ubsan.o bitmap.o kasan.o
subdirs-y += arch/$(ARCH)
