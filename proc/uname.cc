#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "lib/std/string.h"
#include "proc/process.h"
#include "proc/uname.h"

namespace proc {

namespace {

using arch::memory::set_page_directory;
using lib::std::memcpy;
using lib::std::strlen;

} // namespace

uint32_t new_uname(uint32_t name_addr, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
	uint32_t* page_dir = get_currently_executing_process()->page_dir;
	set_page_directory(page_dir);

	struct new_uname_info* name = (struct new_uname_info*)name_addr;

	memcpy(system_name, name->system_name, strlen(system_name)+1);
	memcpy(node_name, name->node_name, strlen(node_name)+1);
	memcpy(release, name->release, strlen(release)+1);
	memcpy(version, name->version, strlen(version)+1);
	memcpy(machine, name->machine, strlen(machine)+1);
	memcpy(domain_name, name->domain_name, strlen(domain_name)+1);

	return 0;
}

} // namespace proc
