export module kernel.platform.x86_64;

import libpara.basic_types;
import kernel.platform;
import kernel.pmm;
import kernel.platform.x86_64.init;
import kernel.platform.x86_64.panic;

using namespace libpara::basic_types;

export namespace kernel::platform {

template <> struct impl<initialize, X86_64> {
  static void function(kernel::pmm::Allocator &allocator) {
    x86_64::initialize(allocator);
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

} // namespace kernel::platform
