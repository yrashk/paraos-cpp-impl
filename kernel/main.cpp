export module kernel.main;

import kernel.devices.serial;
import libpara.basic_types;
import libpara.concepts;
import libpara.formatting;
import kernel.pmm;
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;

using namespace libpara::basic_types;
using namespace libpara::formatting;

export namespace kernel {

class Processor {
protected:
  kernel::pmm::Allocator &allocator;

public:
  Processor(kernel::pmm::Allocator &allocator) : allocator(allocator) {}
  virtual void run() = 0;
};

class BootstrapProcessor : public Processor {

  int initialized = false;
  u16 ncpus;

public:
  BootstrapProcessor(kernel::pmm::Allocator &allocator, u16 ncpus)
      : Processor(allocator), ncpus(ncpus) {}
  BootstrapProcessor(BootstrapProcessor &bsp) = delete;

  virtual void run() {
    kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator);
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    serial.initialize();
    format(serial, "ParaOS\n");
    format(serial, "Available memory: ",
           this->allocator.availableMemory() / (1024 * 1024), "MB\n");
    format(serial, "Number of CPUs: ", ncpus, "\n");
    format(serial, "CPU #",
           kernel::platform::impl<kernel::platform::cpuid>::function(),
           " (BSP) ready\n");

    __atomic_store_n(&initialized, true, __ATOMIC_SEQ_CST);
    kernel::platform::impl<kernel::platform::halt>::function();
  }

  void waitUntilInitialized() {
    while (!__atomic_load_n(&initialized, __ATOMIC_SEQ_CST)) {
      __builtin_ia32_pause();
    }
  }
};

class ApplicationProcessor : public Processor {

  BootstrapProcessor &bsp;

public:
  ApplicationProcessor(kernel::pmm::Allocator &allocator,
                       BootstrapProcessor &bsp)
      : Processor(allocator), bsp(bsp) {}

  virtual void run() {
    bsp.waitUntilInitialized();
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    serial.initialize();
    kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator);
    format(serial, "CPU #",
           kernel::platform::impl<kernel::platform::cpuid>::function(),
           " ready\n");
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

} // namespace kernel
