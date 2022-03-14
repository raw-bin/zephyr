#!/bin/sh
~/Work/repos/qemu/build/qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a53 -m 2048M -nographic -kernel build.lan91c111/zephyr/zephyr.elf
