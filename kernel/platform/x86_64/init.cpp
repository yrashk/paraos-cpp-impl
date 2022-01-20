export module kernel.platform.x86_64.init;

import libpara.err;
import libpara.basic_types;
import libpara.loop;

import kernel.pmm;
import kernel.platform.x86_64.serial;
import kernel.platform.x86_64.panic;
import kernel.platform.x86_64.gdt;
import kernel.platform.x86_64.idt;

using namespace libpara::err;
using namespace libpara::basic_types;
using namespace libpara::loop;

#include <err.hpp>

export namespace kernel::platform::x86_64 {

Result<nothing> initialize(kernel::pmm::Allocator &allocator) {
  using GdtRegister = gdt::Register<2>;
  auto gdtrAlloc =
      tryUnwrap(allocator.allocate(sizeof(GdtRegister), alignof(GdtRegister)));
  auto gdtr = new (gdtrAlloc)
      GdtRegister{.segments = {
                      gdt::Segment(gdt::LongMode, gdt::Code | gdt::Privilege0),
                      gdt::Segment(gdt::Data | gdt::DataWritable),
                  }};

  gdtr->load();

  auto idtrAlloc = tryUnwrap(
      allocator.allocate(sizeof(idt::Register<>), alignof(idt::Register<>)));
  auto idtr = new (idtrAlloc) idt::Register{};

  constexpr_loop<u64, 30>([&]<u64 i>() {
    new (reinterpret_cast<void *>(idtr->gates + i))
        idt::Gate(reinterpret_cast<void *>(
                      kernel::platform::x86_64::panic::panic_isr<i>::isr),
                  gdtr->kernelCodeSegment(), idt::Trap);
  });

  idtr->load();

  return nothing{};
}

} // namespace kernel::platform::x86_64
