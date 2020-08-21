#!/bin/bash
qemu-system-i386 -s -cpu SandyBridge -drive file=test_disk.img -kernel moonshine.bin
