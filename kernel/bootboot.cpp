import libpara.basic_types;
import libpara.err;
import libpara.concepts;
import libpara.formatting;
import kernel.main;
import kernel.pmm;
import kernel.testing;
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;

using namespace libpara::basic_types;
using namespace libpara::err;

struct Bootboot {

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
  static const auto header_bspid = 0x0c;
  static const auto header_mmap = 0x80;

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
        .ptr = reinterpret_cast<void *>(reinterpret_cast<u8 *>(this) +
                                        header_mmap + 16 * index),
        .sz = *reinterpret_cast<u64 *>(reinterpret_cast<u8 *>(this) +
                                       header_mmap + 16 * index + sizeof(u64))};
  }

  bool isBootstrapCPU() {
    return kernel::platform::impl<kernel::platform::cpuid>::function() ==
           bootstrapCPU();
  }
};

extern Bootboot bootboot;

extern "C" void bootboot_main() {
  kernel::pmm::ChainedAllocator<kernel::pmm::WatermarkAllocator, 32>
      defaultAllocator;

  auto bsp = kernel::BootstrapProcessor(defaultAllocator);

  for (u32 i = 0; i < bootboot.mmapEntries(); i++) {
    auto entry = bootboot.mmapEntry(i);
    if (entry.type() == Bootboot::mmap_entry::Free) {
      defaultAllocator.addAllocator(
          kernel::pmm::WatermarkAllocator(entry.ptr, entry.size()));
    }
  }

  if (bootboot.isBootstrapCPU()) {
    bsp.run();
  } else {
    kernel::ApplicationProcessor(defaultAllocator, bsp).run();
  }
}
