#include <stddef.h>
#include <stdint.h>

#include "arch/i386/cpu/save_restore.h"
#include "arch/i386/memory/gdt.h"
#include "arch/i386/memory/paging.h"
#include "arch/interrupts/control.h"
#include "filesystem/file.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/close.h"
#include "proc/process.h"
#include "proc/syscall.h"

namespace proc {

namespace {

using arch::cpu::restore_processor_state;
using arch::interrupts::disable_interrupts;
using arch::interrupts::enable_interrupts;
using arch::memory::enable_paging;
using arch::memory::flush_tss;
using arch::memory::get_page_table_entry;
using arch::memory::main_tss;
using arch::memory::map_memory_segment;
using arch::memory::PAGE_SIZE;
using arch::memory::permission;
using arch::memory::set_page_directory;
using arch::memory::set_tls;
using arch::memory::TLS_ENTRY_OFFSET;
using arch::memory::tls_segment;
using arch::memory::USER_CODE_SELECTOR;
using arch::memory::USER_DATA_SELECTOR;
using arch::memory::user_read_only;
using arch::memory::user_read_write;
using filesystem::file;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::kmalloc_aligned;
using lib::std::make_string_copy;
using lib::std::memcpy;
using lib::std::memset;
using lib::std::panic;
using lib::std::strlen;

volatile struct process *process_list = nullptr;

constexpr uint32_t AT_RANDOM = 25;
constexpr uint64_t STACK_CANARY = 0xDEADBEEFDEADBEEF;

void cleanup_process(struct process *to_cleanup) {
  for (int i = 0; i < to_cleanup->num_segments; i++) {
    kfree(to_cleanup->segments[i].actual_address);
    if (to_cleanup->segments[i].virtual_address) {
      *get_page_table_entry(to_cleanup->page_dir,
                            to_cleanup->segments[i].virtual_address) = 0;
    }
  }
  kfree(to_cleanup->segments);

  struct file *current_file = to_cleanup->open_files;
  while (current_file) {
    close_file(to_cleanup, current_file);
    current_file = to_cleanup->open_files;
  }

  for (int i = 0; i < to_cleanup->num_page_tables; i++) {
    kfree(to_cleanup->page_tables[i]);
  }
  kfree(to_cleanup->page_tables);
  kfree(to_cleanup->page_dir);

  for (int i = 0; i < to_cleanup->argc; i++) {
    if (to_cleanup->argv[i]) {
      kfree(to_cleanup->argv[i]);
    }
  }
  kfree(to_cleanup->argv);

  kfree(to_cleanup->tls_segments);

  kfree(to_cleanup->path);
  kfree(to_cleanup->working_dir);

  if (to_cleanup->next == to_cleanup) {
    process_list = nullptr;
  } else {
    to_cleanup->next->prev = to_cleanup->prev;
    to_cleanup->prev->next = to_cleanup->next;
  }

  kfree(to_cleanup);
}

void execute_new_process(void) {
  void (*code_virtual_start)(void) = process_list->entry;
  uint32_t esp = process_list->esp;
  int argc = process_list->argc;
  char **argv = process_list->argv;

  // Set process state to runnable
  process_list->process_state = RUNNABLE;

  // Set up the temporary thread local storage (TLS)
  set_tls(process_list->tls_segments[process_list->tls_segment_index]);

  // Set the TSS segment to point to this process's kernel stack
  main_tss.esp0 = process_list->kernel_stack_top;
  flush_tss();

  // Setup the MMU to use our process's page tables
  set_page_directory(process_list->page_dir);

  // Setup the process stack and start executing
  asm volatile("movl %0, %%eax\n"
               "movl %1, %%ebx\n"
               "movl %2, %%ecx\n"
               "movl %3, %%edx\n"
               "movw %%dx, %%ds\n"
               "movw %%dx, %%es\n"
               "movl %%eax, %%esp\n"
               "push %%edx\n"
               "push %%eax\n"
               "pushf\n"
               "push %%ecx\n"
               "push %%ebx\n"
               "xor %%eax, %%eax\n"
               "xor %%ebx, %%ebx\n"
               "xor %%ecx, %%ecx\n"
               "xor %%edx, %%edx\n"
               "xor %%edi, %%edi\n"
               "xor %%esi, %%esi\n"
               "xor %%ebp, %%ebp\n"
               "sti\n"
               "iret"
               :
               : "m"(esp), "m"(code_virtual_start), "i"(USER_CODE_SELECTOR),
                 "i"(USER_DATA_SELECTOR));
}

constexpr uint32_t STACK_ALIGNMENT = 0x4;

uint32_t setup_initial_stack(int argc, char **argv, uint32_t stack_top_virtual,
                             char *stack_top_physical) {
  uint32_t setup_size = sizeof(int);
  for (int i = 0; i < argc; i++) {
    setup_size += strlen(argv[i]) + 1 +
                  sizeof(char *); // Add in the size of all the argvs
  }
  setup_size += sizeof(char *); // Argvs are null terminated
  setup_size += sizeof(char *); // Leave room for env
  setup_size +=
      2 * sizeof(uint32_t) + sizeof(uint64_t *) +
      sizeof(uint64_t); // Leave room for an aux vector with stack canary

  // Pad the stack so that we will be aligned
  uint32_t pad_size = STACK_ALIGNMENT - (setup_size % STACK_ALIGNMENT);
  stack_top_virtual -= pad_size;
  stack_top_physical -= pad_size;

  // Initialize the argv
  char **argv_copy = (char **)(stack_top_physical - setup_size + sizeof(int));
  char **argv_copy_virtual =
      (char **)(stack_top_virtual - setup_size + sizeof(int));
  argv_copy[argc + 1] = 0; // Initialize env
  argv_copy[argc] = 0;     // Null terminate argv
  for (int i = argc - 1; i >= 0; i--) {
    size_t argv_size = strlen(argv[i]) + 1;
    stack_top_virtual -= argv_size;
    stack_top_physical -= argv_size;
    memcpy(argv[i], stack_top_physical, argv_size);
    argv_copy[i] = (char *)stack_top_virtual;
  }

  // Initialize aux vector
  stack_top_virtual -= sizeof(uint64_t);
  stack_top_physical -= sizeof(uint64_t);
  uint64_t *canary_data_physical = (uint64_t *)stack_top_physical;
  uint64_t *canary_data_virtual = (uint64_t *)stack_top_virtual;
  *canary_data_physical = STACK_CANARY;
  stack_top_virtual -= sizeof(uint32_t);
  stack_top_physical -= sizeof(uint32_t);
  *(uint32_t *)stack_top_physical = 0;
  stack_top_virtual -= sizeof(uint64_t *);
  stack_top_physical -= sizeof(uint64_t *);
  *(uint64_t **)stack_top_physical = canary_data_virtual;
  stack_top_virtual -= sizeof(uint32_t);
  stack_top_physical -= sizeof(uint32_t);
  *(uint32_t *)stack_top_physical = AT_RANDOM;
  uint32_t *aux_vector = (uint32_t *)stack_top_virtual;

  // We already populated argv above, skip these bytes
  stack_top_virtual -= (argc + 2) * sizeof(char *);
  stack_top_physical -= (argc + 2) * sizeof(char *);

  // Note that we don't actually push a pointer to argv or auxvector onto the
  // stack, even though it looks like we should based on _libc_start_main's
  // source code. I guess this is handled by start.S?

  // Initialize the argc
  stack_top_virtual -= sizeof(int);
  stack_top_physical -= sizeof(int);
  *(int *)stack_top_physical = argc;

  return stack_top_virtual;
}

// Used for temporarily ensuring that syscalls can be made before TLS is setup
extern "C" void raw_syscall();

asm volatile("raw_syscall:\n"
             "int $0x80\n"
             "ret");

uint32_t next_pid = 1;

} // namespace

char spawn_new_process(char *path, int argc, char **argv,
                       struct process_memory_segment *segments,
                       uint32_t num_segments, void (*entry_address)(void),
                       char *working_dir) {
  disable_interrupts();

  // Check to make sure we aren't allocating kernel memory
  for (int i = 0; i < num_segments; i++) {
    if ((uint32_t)segments[i].virtual_address < lib::std::END_OF_KERNEL) {
      return 0;
    }
  }

  struct process *new_proc = (struct process *)kmalloc(sizeof(struct process));

  new_proc->pid = assign_pid();

  new_proc->path = make_string_copy(path);
  new_proc->working_dir = make_string_copy(working_dir);

  // Copy the segment map into the process struct
  new_proc->num_segments = num_segments + 2;
  new_proc->segments = (struct process_memory_segment *)kmalloc(
      new_proc->num_segments * sizeof(struct process_memory_segment));
  memcpy((char *)segments, (char *)new_proc->segments,
         num_segments * sizeof(struct process_memory_segment));

  // Add a stack segment
  uint32_t stack_bottom = DEFAULT_STACK_BOTTOM;
  for (int i = 0; i < num_segments; i++) {
    if ((uint32_t)segments[i].virtual_address < stack_bottom) {
      stack_bottom = (uint32_t)segments[i].virtual_address;
    }
  }
  stack_bottom -= DEFAULT_STACK_SIZE;
  struct process_memory_segment *stack_segment =
      new_proc->segments + (new_proc->num_segments - 2);
  stack_segment->virtual_address = (void *)stack_bottom;
  stack_segment->segment_size = DEFAULT_STACK_SIZE;
  stack_segment->disk_size = 0;
  stack_segment->source = nullptr;
  stack_segment->flags = READABLE_MEMORY || WRITEABLE_MEMORY;
  new_proc->esp =
      ((uint32_t)stack_segment->virtual_address + stack_segment->segment_size) &
      0xFFFFFFFC; // Stacks are generally 4 byte aligned

  // Add a kernel stack segment
  struct process_memory_segment *kernel_stack_segment =
      new_proc->segments + (new_proc->num_segments - 1);
  kernel_stack_segment->virtual_address =
      nullptr; // Let's make sure the kernel stack has the same virtual address
               // as it does physical
  kernel_stack_segment->segment_size = DEFAULT_STACK_SIZE;
  kernel_stack_segment->disk_size = 0;
  kernel_stack_segment->source = nullptr;
  kernel_stack_segment->flags = READABLE_MEMORY | WRITEABLE_MEMORY;

  // Allocate memory segments and copy disk data into them
  for (int i = 0; i < new_proc->num_segments; i++) {
    size_t page_offset =
        ((uint32_t)new_proc->segments[i].virtual_address & (PAGE_SIZE - 1));
    size_t alloc_size = new_proc->segments[i].segment_size + page_offset;
    alloc_size = (alloc_size & (PAGE_SIZE - 1))
                     ? ((alloc_size / PAGE_SIZE) + 1) * PAGE_SIZE
                     : alloc_size;
    new_proc->segments[i].alloc_size = alloc_size;
    new_proc->segments[i].actual_address =
        kmalloc_aligned(alloc_size, PAGE_SIZE);
    memset((char *)new_proc->segments[i].actual_address, alloc_size, 0);

    if (new_proc->segments[i].virtual_address &&
        new_proc->segments[i].flags == (READABLE_MEMORY | WRITEABLE_MEMORY)) {
      new_proc->brk = ((uint32_t)new_proc->segments[i].virtual_address &
                       (~(PAGE_SIZE - 1))) +
                      alloc_size;
      new_proc->actual_brk = new_proc->brk;
    }

    if (new_proc->segments[i].source != nullptr) {
      memcpy((char *)new_proc->segments[i].source,
             (char *)new_proc->segments[i].actual_address + page_offset,
             new_proc->segments[i].disk_size);
    }
  }

  new_proc->lower_brk = new_proc->brk / 2;

  new_proc->kernel_stack_top = ((uint32_t)kernel_stack_segment->actual_address +
                                kernel_stack_segment->segment_size) &
                               0xFFFFFFFC;

  // Add a temporary TLS
  // There's a bug in GNU libc that lets it make software syscalls before it
  // sets up the TLS under specific circumstances So we just add a temporary TLS
  // with the right syscall info :P
  // TODO: maybe file a bug report?
  struct tls_segment *temp_tls =
      (struct tls_segment *)kmalloc(sizeof(struct tls_segment));
  temp_tls->gdt_index = TLS_ENTRY_OFFSET;
  temp_tls->segment_base = (uint32_t)stack_segment->virtual_address;
  temp_tls->limit = 0xFF;
  uint32_t raw_syscall_copy_offset = 0x100;
  memcpy((char *)raw_syscall,
         (char *)(stack_segment->actual_address + raw_syscall_copy_offset),
         3); // Kernel memory isn't readable from userspace, so we just copy it
             // to a random address near the bottom of the stack
  *(uint32_t *)((uint32_t)stack_segment->actual_address + 0x10) =
      (uint32_t)(stack_segment->virtual_address +
                 raw_syscall_copy_offset); // Got this magic number from a
                                           // disassembly dump of libc
  new_proc->tls_segments = temp_tls;
  new_proc->num_tls_segments = 1;
  new_proc->tls_segment_index = 0;

  // Set up the initial stack with argc and argv
  uint32_t stack_top_physical = new_proc->esp -
                                (uint32_t)stack_segment->virtual_address +
                                (uint32_t)stack_segment->actual_address;
  new_proc->argc = argc;
  new_proc->argv = argv;
  new_proc->esp =
      setup_initial_stack(new_proc->argc, new_proc->argv, new_proc->esp,
                          (char *)stack_top_physical);

  // Set up page directory
  new_proc->page_dir =
      (uint32_t *)kmalloc_aligned(1024 * sizeof(uint32_t), PAGE_SIZE);
  memcpy((char *)base_page_directory, (char *)new_proc->page_dir,
         2 * sizeof(uint32_t));
  memset((char *)(new_proc->page_dir + 2), 1022 * sizeof(uint32_t), 0);

  // Set up memory segment page tables
  new_proc->num_page_tables = 0;
  new_proc->page_tables = nullptr;
  for (int i = 0; i < new_proc->num_segments; i++) {
    uint32_t virtual_address = (uint32_t)new_proc->segments[i].virtual_address;
    if (!virtual_address) {
      virtual_address = (uint32_t)new_proc->segments[i].actual_address;
    }
    size_t page_offset = virtual_address & (PAGE_SIZE - 1);
    size_t alloc_size = new_proc->segments[i].alloc_size;
    uint32_t num_pages = alloc_size / PAGE_SIZE;
    if (alloc_size & (PAGE_SIZE - 1)) {
      num_pages++;
    }
    map_memory_segment(new_proc, (uint32_t)new_proc->segments[i].actual_address,
                       virtual_address - page_offset, alloc_size,
                       user_read_write);
  }

  // Initialize the file descriptors
  new_proc->open_files = nullptr;
  new_proc->next_file_descriptor = 3; // STDOUT and STDIN are 0 and 1

  // Set process as runnable
  new_proc->process_state = NEW;

  // Initialize the wait reason
  new_proc->wait = nullptr;

  // Set the virtual address to start at
  new_proc->entry = entry_address;

  // Insert process into linked list
  if (process_list == nullptr) {
    process_list = new_proc;
    process_list->next = (struct process *)process_list;
    process_list->prev = (struct process *)process_list;
  } else {
    new_proc->next = process_list->next;
    new_proc->prev = (struct process *)process_list;
    new_proc->next->prev = new_proc;
    process_list->next = new_proc;
  }

  return 1;
}

void __attribute__((naked)) execute_processes(void) {
  asm volatile("mov $stack_top, %esp\n"
               "mov %esp, %ebp");

  while (process_list) {
    if (process_list->process_state == STOPPED) {
      process_list = process_list->next;
      cleanup_process(process_list->prev);
    } else if (process_list->process_state == RUNNABLE) {
      uint32_t esp = process_list->esp;
      uint32_t kernel_esp = process_list->kernel_stack_top;
      set_tls(process_list->tls_segments[process_list->tls_segment_index]);
      set_page_directory(process_list->page_dir);
      restore_processor_state(esp, kernel_esp);
      process_list->process_state =
          STOPPED; // This will only happen if the process exited
      process_list = process_list->next;
    } else if (process_list->process_state == NEW) {
      execute_new_process();
    } else if (process_list->process_state == WAITING) {
      struct process *current_proc = (struct process *)process_list;

      // Find next non-waiting process
      process_list = process_list->next;
      while (process_list != current_proc &&
             process_list->process_state == WAITING) {
        process_list = process_list->next;
      }

      // All processes are waiting, hlt to save power
      if (process_list == current_proc) {
        main_tss.esp0 = (uint32_t)&stack_top;
        flush_tss();
        asm volatile("sti\n"
                     "hlt\n");
      }
    } else {
      panic("Invalid process state!");
    }
  }

  while (1) {
    asm volatile("cli\n"
                 "hlt");
  }
}

struct process *get_currently_executing_process(void) {
  return (struct process *)process_list;
}

void advance_process_queue(void) {
  if (process_list) {
    process_list = process_list->next;
  }
}

void set_userspace_page_table(void) {
  struct process *current_process = get_currently_executing_process();
  set_page_directory(current_process->page_dir);
}

uint32_t assign_pid(void) {
  uint32_t ret = next_pid;
  next_pid++;
  return ret;
}

void kill_current_process(void) {
  get_currently_executing_process()->process_state = STOPPED;
  execute_processes();
}

} // namespace proc
