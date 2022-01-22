export module kernel.platform.x86_64;

import libpara.basic_types;
import libpara.err;

import kernel.platform;
import kernel.pmm;
import kernel.platform.x86_64.init;
import kernel.platform.x86_64.panic;

using namespace libpara::basic_types;
using namespace libpara::err;

#include <err.hpp>

export namespace kernel::platform {

template <> struct impl<initialize, X86_64> {
  static Result<nothing> function(kernel::pmm::Allocator &allocator) {
    return tryUnwrap(x86_64::initialize(allocator));
  }
};

template <> struct impl<cpuid, X86_64> {
  static u16 function() {
    u32 ebx;
    asm volatile("mov $1, %%eax; cpuid; mov %%ebx, %0" : "=r"(ebx));
    return ebx >> 24;
  }
};

template <> struct impl<halt, X86_64> {
  static void function() { asm("cli ; hlt"); }
};

template <> struct impl<exit_emulator, X86_64> {
  [[noreturn]] static void function(u16 exit_code) {
    asm volatile(
        "mov %0, %%ax ; mov %1, %%dx; outw %%ax, %%dx" ::"a"(exit_code),
        "i"(0x501));
    while (true) {
    }
  }
};

} // namespace kernel::platform
