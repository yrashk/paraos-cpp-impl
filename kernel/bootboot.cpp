export module kernel.bootboot;

import libpara.basic_types;
import libpara.err;
import libpara.loop;
import libpara.concepts;
import libpara.formatting;
import kernel.main;
import kernel.pmm;
#ifndef RELEASE
import kernel.testing;
#endif
import kernel.devices.serial;
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;
import kernel.platform.x86_64.init;

using namespace libpara::basic_types;
using namespace libpara::err;
using namespace libpara::formatting;

#include <err.hpp>

export struct Bootboot {

  struct mmap_entry {
    void *ptr;
    usize sz;
    using mmap_type = enum {
      Used = 0,
      Free = 1,
      ACPI = 2,
      MMIO = 3,
    };

    inline usize size() { return sz & 0xFFFFFFFFFFFFFFF0; }
    inline mmap_type type() { return static_cast<mmap_type>(sz & 0xf); }
  };

  static const auto header_size = 0x04;
  static const auto header_numcores = 0x0a;
  static const auto header_bspid = 0x0c;
  static const auto header_mmap = 0x80;

  inline u16 numCores() {
    return *reinterpret_cast<u16 *>(reinterpret_cast<u8 *>(this) +
                                    header_numcores);
  }

  u16 bootstrapCPU() {
    return *reinterpret_cast<u16 *>(reinterpret_cast<u8 *>(this) +
                                    header_bspid);
  }

  u32 size() {
    return *reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(this) + header_size);
  }

  inline u32 mmapEntries() { return (size() - 128) / 16; }

  inline mmap_entry mmapEntry(u32 index) {
    return mmap_entry{
        .ptr = *reinterpret_cast<void **>(reinterpret_cast<u8 *>(this) +
                                          header_mmap + 16 * index),
        .sz = *reinterpret_cast<u64 *>(reinterpret_cast<u8 *>(this) +
                                       header_mmap + 16 * index + sizeof(u64))};
  }

  bool isBootstrapCPU() {
    return kernel::platform::impl<kernel::platform::cpuid>::function() ==
           bootstrapCPU();
  }
};

export extern Bootboot bootboot;
export extern unsigned char environment[4096];

#ifndef RELEASE
bool isTesting() {
  usize i = 0;
  while (true) {
    if (environment[i] == 0)
      return false;
    if (environment[i] == 't' && environment[i + 1] == 'e' &&
        environment[i + 2] == 's' && environment[i + 3] == 't' &&
        environment[i + 4] == '=' && environment[i + 5] == 'y' &&
        environment[i + 6] == 'e' && environment[i + 7] == 's')
      return true;
    i++;
  }
}
#endif

export extern "C" void bootboot_main() {
  static constinit kernel::pmm::ChainedAllocator<
      kernel::pmm::WatermarkAllocator, 32>
      defaultAllocator;

  static constinit auto bsp = kernel::BootstrapProcessor(defaultAllocator);

#ifndef RELEASE
  if (isTesting()) {
    if (bootboot.isBootstrapCPU()) {
      kernel::testing::run();
    }
    kernel::platform::impl<kernel::platform::halt>::function();
  }
#endif
  if (bootboot.isBootstrapCPU()) {
    bsp.setNumCPUs(bootboot.numCores());

    for (u32 i = 0; i < bootboot.mmapEntries(); i++) {
      auto entry = bootboot.mmapEntry(i);
      if (entry.type() == Bootboot::mmap_entry::Free) {
        tryCatch(
            defaultAllocator.addAllocator(
                kernel::pmm::WatermarkAllocator(entry.ptr, entry.size())),
            err, ({
              kernel::platform::impl<kernel::devices::SerialPort>::type serial;
              serial.initialize();
              format(serial, "Uncaught error while preparing PMM: ", err.name,
                     "\n");
              0;
            }));
      }
    }
    bsp.start();
  } else {
    kernel::ApplicationProcessor(defaultAllocator, bsp).start();
  }
}
