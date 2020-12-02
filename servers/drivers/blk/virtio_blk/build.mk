name := virtio_blk
description := A virtio-blk block device driver
objs-y := main.o
libs-y := virtio driver

QEMUFLAGS += -drive if=none,file=$(BUILD_DIR)/virtio_blk.img,id=drive0
QEMUFLAGS += -device virtio-blk,drive=drive0,packed=on
