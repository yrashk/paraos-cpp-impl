import libpara.basic_types;
import kernel.main;
import kernel.platform;
import kernel.platform.x86_64;

using namespace libpara::basic_types;

struct Bootboot {

  static const auto bspid = 0x0c;

  short bootstrapCPU() {
    return *reinterpret_cast<u16 *>(reinterpret_cast<u8 *>(this) + bspid);
  }

  bool isBootstrapCPU() {
    return kernel::platform::impl<kernel::platform::cpuid>::function() ==
           bootstrapCPU();
  }
};

extern Bootboot bootboot;

auto bsp = kernel::BootstrapProcessor();

extern "C" void bootboot_main() {
  if (bootboot.isBootstrapCPU()) {
    bsp.run();
  } else {
    kernel::ApplicationProcessor(bsp).run();
  }
}
