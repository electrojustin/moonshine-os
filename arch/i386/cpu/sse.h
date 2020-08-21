#ifndef ARCH_CPU_SSE_H
#define ARCH_CPU_SSE_H

namespace arch {
namespace cpu {

extern "C" char is_sse_enabled;

void maybe_enable_sse(void);

} // namespace cpu
} // namespace arch

#endif
