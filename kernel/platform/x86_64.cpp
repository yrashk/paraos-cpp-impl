export module kernel.platform.x86_64;

import libpara.basic_types;
import kernel.platform;

using namespace libpara::basic_types;

export namespace kernel::platform {

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

} // namespace kernel::platform
