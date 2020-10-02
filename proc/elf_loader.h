#ifndef PROC_ELF_LOADER_H
#define PROC_ELF_LOADER_H

#include "filesystem/file.h"

namespace proc {

namespace {

using filesystem::file_descriptor;

} // namespace

char load_elf(char *path, int argc, char **argv, char *working_dir = "/",
              struct file_descriptor *standard_in = nullptr,
              struct file_descriptor *standard_out = nullptr,
              struct file_descriptor *standard_error = nullptr,
              struct file_descriptor *open_files = nullptr,
              uint32_t next_file_descriptor = 3);

} // namespace proc

#endif
