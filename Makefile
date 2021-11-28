# Makefile - makefile of our first driver

obj-m := block_driver_new.o
KERNEL_SOURCE := /usr/src/kernels/$(shell uname -r)
PWD := $(shell pwd)

default:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules

clean:
	${MAKE} -C "${KERNEL_SOURCE}" M="${PWD}" clean
test:
	sudo dmesg -C
	sudo insmod new_block_driver.ko
	sudo rmmod new_block_driver.ko
	dmesg
