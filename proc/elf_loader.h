#ifndef PROC_ELF_LOADER_H
#define PROC_ELF_LOADER_H

namespace proc {

char load_elf(char* path, int argc, char** argv, char* working_dir="/");

} // namespace proc

#endif
