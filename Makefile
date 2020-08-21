CFLAGS="-m32" "-ffreestanding" "-mno-red-zone" "-fno-exceptions" "-fno-rtti" "-nostdlib" "-I$(shell pwd)"
USERSPACE_CFLAGS="-m32" "-static" "-fPIE"
all: moonshine.bin
moonshine.bin: boot.o \
	       main.o \
	       arch/cpu/model_specific.o \
	       arch/cpu/save_restore.o \
	       arch/cpu/sse.o \
	       arch/interrupts/apic.o \
	       arch/interrupts/control.o \
	       arch/interrupts/interrupts.o \
	       arch/interrupts/pic.o \
	       arch/memory/gdt.o \
	       arch/memory/paging.o \
	       drivers/keyboard.o \
	       drivers/pata.o \
	       drivers/pci.o \
	       drivers/pit.o \
	       filesystem/chs.o \
	       filesystem/fat32.o \
	       filesystem/file.o \
	       filesystem/mbr.o \
	       io/keyboard.o \
	       io/io.o \
	       io/vga.o \
	       lib/std/memory.o \
	       lib/std/stdio.o \
	       lib/std/string.o \
	       lib/std/time.o \
	       proc/arch.o \
	       proc/brk.o \
	       proc/close.o \
	       proc/dir.o \
	       proc/elf_loader.o \
	       proc/execve.o \
	       proc/exit.o \
	       proc/fork.o \
	       proc/ioctl.o \
	       proc/open.o \
	       proc/pid.o \
	       proc/process.o \
	       proc/read_write.o \
	       proc/sleep.o \
	       proc/stat.o \
	       proc/syscall.o \
	       proc/thread_area.o \
	       proc/uid.o \
	       proc/uname.o \
	       linker.ld
	gcc $(CFLAGS) boot.o \
		      main.o \
		      arch/cpu/model_specific.o \
		      arch/cpu/save_restore.o \
		      arch/cpu/sse.o \
		      arch/interrupts/apic.o \
		      arch/interrupts/control.o \
	              arch/interrupts/interrupts.o \
		      arch/interrupts/pic.o \
		      arch/memory/gdt.o \
		      arch/memory/paging.o \
		      drivers/keyboard.o \
		      drivers/pata.o \
		      drivers/pci.o \
		      drivers/pit.o \
		      filesystem/chs.o \
		      filesystem/fat32.o \
		      filesystem/file.o \
		      filesystem/mbr.o \
		      io/keyboard.o \
		      io/io.o \
		      io/vga.o \
		      lib/std/memory.o \
		      lib/std/stdio.o \
		      lib/std/string.o \
		      lib/std/time.o \
		      proc/arch.o \
		      proc/brk.o \
		      proc/close.o \
		      proc/dir.o \
		      proc/elf_loader.o \
		      proc/execve.o \
		      proc/exit.o \
		      proc/fork.o \
		      proc/ioctl.o \
		      proc/open.o \
		      proc/pid.o \
		      proc/process.o \
		      proc/read_write.o \
		      proc/sleep.o \
		      proc/stat.o \
		      proc/syscall.o \
		      proc/thread_area.o \
		      proc/uid.o \
		      proc/uname.o -T linker.ld -o moonshine.bin
boot.o: boot.s
	gcc $(CFLAGS) -c boot.s
main.o: main.cc \
	     arch/i386/cpu/sse.h \
	     arch/i386/interrupts/apic.h \
	     arch/i386/interrupts/pic.h \
	     arch/i386/memory/gdt.h \
	     arch/i386/memory/meminfo.h \
	     arch/i386/memory/paging.h \
	     arch/i386/multiboot.h \
	     arch/interrupts/control.h \
	     arch/interrupts/interrupts.h \
	     drivers/i386/keyboard.h \
	     drivers/i386/pata.h \
	     drivers/i386/pci.h \
	     drivers/i386/pit.h \
	     filesystem/fat32.h \
	     filesystem/mbr.h \
	     io/vga.h \
	     lib/std/memory.h \
	     lib/std/stdio.h \
	     lib/std/string.h \
	     proc/arch.h \
	     proc/brk.h \
	     proc/close.h \
	     proc/dir.h \
	     proc/elf_loader.h \
	     proc/execve.h \
	     proc/exit.h \
	     proc/fork.h \
	     proc/ioctl.h \
	     proc/open.h \
	     proc/pid.h \
	     proc/process.h \
	     proc/read_write.h \
	     proc/sleep.h \
	     proc/stat.h \
	     proc/syscall.h \
	     proc/thread_area.h \
	     proc/uid.h \
	     proc/uname.h
	gcc $(CFLAGS) -c main.cc
arch/cpu/model_specific.o: arch/i386/cpu/model_specific.cc \
			   arch/i386/cpu/model_specific.h
	gcc $(CFLAGS) -c arch/i386/cpu/model_specific.cc -o arch/cpu/model_specific.o
arch/cpu/save_restore.o: arch/i386/cpu/save_restore.cc \
			 arch/i386/cpu/save_restore.h \
			 arch/i386/memory/gdt.h \
			 arch/i386/memory/paging.h \
			 proc/process.h
	gcc $(CFLAGS) -c arch/i386/cpu/save_restore.cc -o arch/cpu/save_restore.o
arch/cpu/sse.o: arch/i386/cpu/sse.cc \
		arch/i386/cpu/sse.h
	gcc $(CFLAGS) -c arch/i386/cpu/sse.cc -o arch/cpu/sse.o
arch/interrupts/apic.o: arch/i386/interrupts/apic.cc \
			arch/i386/interrupts/apic.h \
			arch/i386/cpu/model_specific.h
	gcc $(CFLAGS) -c arch/i386/interrupts/apic.cc -o arch/interrupts/apic.o
arch/interrupts/control.o: arch/i386/interrupts/control.cc \
			   arch/interrupts/control.h
	gcc $(CFLAGS) -c arch/i386/interrupts/control.cc -o arch/interrupts/control.o
arch/interrupts/interrupts.o: arch/i386/interrupts/interrupts.cc \
			      arch/interrupts/interrupts.h \
			      arch/i386/interrupts/error_interrupts.h \
			      arch/i386/interrupts/idt.h \
			      arch/i386/interrupts/pic.h \
			      arch/i386/memory/gdt.h \
			      lib/std/stdio.h
	gcc $(CFLAGS) -mgeneral-regs-only -c arch/i386/interrupts/interrupts.cc -o arch/interrupts/interrupts.o
arch/interrupts/pic.o: arch/i386/interrupts/pic.cc \
		       arch/i386/interrupts/pic.h \
		       io/i386/io.h
	gcc $(CFLAGS) -c arch/i386/interrupts/pic.cc -o arch/interrupts/pic.o
arch/memory/gdt.o: arch/i386/memory/gdt.cc \
		   arch/i386/memory/gdt.h
	gcc $(CFLAGS) -c arch/i386/memory/gdt.cc -o arch/memory/gdt.o
arch/memory/paging.o: arch/i386/memory/paging.cc \
		      arch/i386/memory/paging.h \
		      lib/std/memory.h \
		      lib/std/stdio.h \
		      proc/process.h
	gcc $(CFLAGS) -c arch/i386/memory/paging.cc -o arch/memory/paging.o
drivers/keyboard.o: drivers/i386/keyboard.cc \
		    drivers/i386/keyboard.h \
		    arch/interrupts/interrupts.h \
		    arch/i386/interrupts/idt.h \
		    arch/i386/interrupts/pic.h \
		    io/i386/io.h \
		    io/keyboard.h
	gcc $(CFLAGS) -mgeneral-regs-only -c drivers/i386/keyboard.cc -o drivers/keyboard.o
drivers/pata.o: drivers/i386/pata.cc \
		drivers/i386/pata.h \
		io/i386/io.h \
		lib/std/memory.h
	gcc $(CFLAGS) -c drivers/i386/pata.cc -o drivers/pata.o
drivers/pci.o: drivers/i386/pci.cc \
	       drivers/i386/pci.h \
	       lib/std/memory.h
	gcc $(CFLAGS) -c drivers/i386/pci.cc -o drivers/pci.o
drivers/pit.o: drivers/i386/pit.cc \
	       drivers/i386/pit.h \
	       arch/i386/cpu/save_restore.h \
	       arch/i386/interrupts/idt.h \
	       arch/i386/interrupts/pic.h \
	       arch/interrupts/control.h \
	       arch/interrupts/interrupts.h \
	       drivers/i386/pit.h \
	       io/i386/io.h \
	       lib/math.h \
	       lib/std/memory.h \
	       lib/std/stdio.h \
	       lib/std/time.h \
	       proc/process.h \
	       proc/sleep.h
	gcc $(CFLAGS) -mgeneral-regs-only -c drivers/i386/pit.cc -o drivers/pit.o
filesystem/chs.o: filesystem/chs.cc \
		  filesystem/chs.h \
		  drivers/i386/pata.h
	gcc $(CFLAGS) -c filesystem/chs.cc -o filesystem/chs.o
filesystem/fat32.o: filesystem/fat32.cc \
		    filesystem/fat32.h \
		    drivers/i386/pata.h \
		    filesystem/mbr.h \
		    lib/std/memory.h \
		    lib/std/string.h
	gcc $(CFLAGS) -c filesystem/fat32.cc -o filesystem/fat32.o
filesystem/file.o: filesystem/file.cc \
		   filesystem/file.h \
		   filesystem/fat32.h \
		   lib/std/memory.h \
		   lib/std/string.h
	gcc $(CFLAGS) -c filesystem/file.cc -o filesystem/file.o
filesystem/mbr.o: filesystem/mbr.cc \
		  filesystem/mbr.h \
		  drivers/i386/pata.h \
		  filesystem/chs.h \
		  lib/std/stdio.h
	gcc $(CFLAGS) -c filesystem/mbr.cc -o filesystem/mbr.o
io/keyboard.o: io/keyboard.cc \
	       io/keyboard.h \
	       lib/std/memory.h \
	       lib/std/stdio.h \
	       proc/process.h
	gcc $(CFLAGS) -c io/keyboard.cc -o io/keyboard.o
io/io.o: io/i386/io.cc \
	 io/i386/io.h
	gcc $(CFLAGS) -c io/i386/io.cc -o io/io.o
io/vga.o: io/i386/vga.cc \
	  io/vga.h
	gcc $(CFLAGS) -c io/i386/vga.cc -o io/vga.o
lib/std/memory.o: lib/std/memory.cc \
		  lib/std/memory.h \
		  lib/std/stdio.h \
		  arch/interrupts/control.h
	gcc $(CFLAGS) -c lib/std/memory.cc -o lib/std/memory.o
lib/std/stdio.o: lib/std/stdio.cc \
		 lib/std/stdio.h \
		 io/vga.h \
		 io/keyboard.h \
		 lib/std/memory.h \
		 lib/std/string.h
	gcc $(CFLAGS) -c lib/std/stdio.cc -o lib/std/stdio.o
lib/std/string.o: lib/std/string.cc \
		  lib/std/string.h \
		  lib/std/memory.h
	gcc $(CFLAGS) -c lib/std/string.cc -o lib/std/string.o
lib/std/time.o: lib/std/time.cc \
		lib/std/time.h
	gcc $(CFLAGS) -c lib/std/time.cc -o lib/std/time.o
proc/arch.o: proc/arch.cc \
	     proc/arch.h 
	gcc $(CFLAGS) -c proc/arch.cc -o proc/arch.o
proc/elf_loader.o: proc/elf_loader.cc \
		   proc/elf_loader.h \
		   filesystem/fat32.h \
		   lib/std/memory.h \
		   proc/process.h
	gcc $(CFLAGS) -c proc/elf_loader.cc -o proc/elf_loader.o
proc/execve.o: proc/execve.cc \
	       proc/execve.h \
	       arch/i386/memory/paging.h \
	       proc/elf_loader.h \
	       proc/process.h
	gcc $(CFLAGS) -c proc/execve.cc -o proc/execve.o
proc/exit.o: proc/exit.cc \
	     proc/exit.h \
	     proc/process.h
	gcc $(CFLAGS) -c proc/exit.cc -o proc/exit.o
proc/brk.o: proc/brk.cc \
	    proc/brk.h \
	    arch/i386/memory/paging.h \
	    proc/process.h \
	    lib/std/memory.h
	gcc $(CFLAGS) -c proc/brk.cc -o proc/brk.o
proc/close.o: proc/close.cc \
	      proc/close.h \
	      filesystem/file.h \
	      lib/std/memory.h \
	      proc/process.h
	gcc $(CFLAGS) -c proc/close.cc -o proc/close.o
proc/dir.o: proc/dir.cc \
	    proc/dir.h \
	    arch/i386/memory/paging.h \
	    filesystem/fat32.h \
	    lib/std/memory.h \
	    lib/std/string.h \
	    proc/process.h \
	    proc/read_write.h
	gcc $(CFLAGS) -c proc/dir.cc -o proc/dir.o
proc/fork.o: proc/fork.cc \
	     proc/fork.h \
	     arch/i386/cpu/sse.h \
	     arch/i386/memory/gdt.h \
	     filesystem/file.h \
	     lib/std/memory.h \
	     lib/std/string.h \
	     proc/process.h
	gcc $(CFLAGS) -c proc/fork.cc -o proc/fork.o
proc/ioctl.o: proc/ioctl.cc \
	      proc/ioctl.h
	gcc $(CFLAGS) -c proc/ioctl.cc -o proc/ioctl.o
proc/open.o: proc/open.cc \
	     proc/open.h \
	     arch/i386/memory/paging.h \
	     filesystem/fat32.h \
	     filesystem/file.h \
	     lib/std/memory.h \
	     lib/std/string.h \
	     proc/process.h
	gcc $(CFLAGS) -c proc/open.cc -o proc/open.o
proc/pid.o: proc/pid.cc \
	    proc/pid.h \
	    proc/process.h
	gcc $(CFLAGS) -c proc/pid.cc -o proc/pid.o
proc/process.o: proc/process.cc \
		proc/process.h \
		arch/i386/cpu/save_restore.h \
		arch/i386/memory/gdt.h \
		arch/i386/memory/paging.h \
		arch/interrupts/control.h \
		filesystem/file.h \
		lib/std/memory.h \
		lib/std/string.h \
		proc/close.h \
		proc/syscall.h
	gcc $(CFLAGS) -c proc/process.cc -o proc/process.o
proc/read_write.o: proc/read_write.cc \
		   proc/read_write.h \
		   arch/i386/memory/paging.h \
		   filesystem/fat32.h \
		   filesystem/file.h \
		   io/keyboard.h \
		   lib/std/memory.h \
		   lib/std/stdio.h \
		   lib/std/string.h \
		   proc/process.h 
	gcc $(CFLAGS) -c proc/read_write.cc -o proc/read_write.o
proc/sleep.o: proc/sleep.cc \
	      proc/sleep.h \
	      arch/i386/memory/paging.h \
	      lib/std/memory.h \
	      lib/std/time.h \
	      proc/process.h
	gcc $(CFLAGS) -c proc/sleep.cc -o proc/sleep.o
proc/stat.o: proc/stat.cc \
	     proc/stat.h
	gcc $(CFLAGS) -c proc/stat.cc -o proc/stat.o
proc/syscall.o: proc/syscall.h \
		proc/i386/syscall.cc \
		arch/i386/cpu/save_restore.h \
		arch/i386/interrupts/idt.h \
		arch/i386/memory/paging.h \
		arch/interrupts/interrupts.h \
		lib/std/memory.h \
		lib/std/stdio.h \
		proc/syscall.h
	gcc $(CFLAGS) -mgeneral-regs-only -c proc/i386/syscall.cc -o proc/syscall.o
proc/thread_area.o: proc/thread_area.h \
		    proc/thread_area.cc \
		    arch/i386/memory/gdt.h \
		    arch/i386/memory/paging.h \
		    lib/std/memory.h \
		    proc/process.h
	gcc $(CFLAGS) -c proc/thread_area.cc -o proc/thread_area.o
proc/uid.o: proc/uid.h \
	    proc/uid.cc
	gcc $(CFLAGS) -c proc/uid.cc -o proc/uid.o
proc/uname.o: proc/uname.h \
	      proc/uname.cc \
	      arch/i386/memory/paging.h \
	      lib/std/memory.h \
	      lib/std/string.h \
	      proc/process.h
	gcc $(CFLAGS) -c proc/uname.cc -o proc/uname.o
userspace/init: userspace/init.cc
	gcc $(USERSPACE_CFLAGS) userspace/init.cc -o userspace/init
userspace/test: userspace/test.cc
	gcc $(USERSPACE_CFLAGS) userspace/test.cc -o userspace/test
clean:
	rm boot.o \
	main.o \
	arch/cpu/model_specific.o \
	arch/cpu/save_restore.o \
	arch/cpu/sse.o \
	arch/memory/gdt.o \
	arch/memory/paging.o \
	arch/interrupts/apic.o \
	arch/interrupts/control.o \
	arch/interrupts/interrupts.o \
	arch/interrupts/pic.o \
	drivers/keyboard.o \
	drivers/pata.o \
	drivers/pci.o \
	drivers/pit.o \
	filesystem/chs.o \
	filesystem/fat32.o \
	filesystem/file.o \
	filesystem/mbr.o \
	io/keyboard.o \
	io/io.o \
	io/vga.o \
	lib/std/memory.o \
	lib/std/stdio.o \
	lib/std/string.o \
	lib/std/time.o \
	proc/arch.o \
	proc/brk.o \
	proc/close.o \
	proc/dir.o \
	proc/elf_loader.o \
	proc/execve.o \
	proc/exit.o \
	proc/fork.o \
	proc/ioctl.o \
	proc/open.o \
	proc/pid.o \
	proc/process.o \
	proc/read_write.o \
	proc/sleep.o \
	proc/stat.o \
	proc/syscall.o \
	proc/thread_area.o \
	proc/uid.o \
	proc/uname.o
