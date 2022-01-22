export module kernel.main;

import libpara.basic_types;
import libpara.concepts;
import libpara.formatting;
import libpara.err;

import kernel.devices.serial;
import kernel.pmm;
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;

using namespace libpara::basic_types;
using namespace libpara::formatting;
using namespace libpara::err;

#include <err.hpp>

export namespace kernel {

class Processor {
protected:
  kernel::pmm::Allocator &allocator;

public:
  constexpr Processor(kernel::pmm::Allocator &allocator)
      : allocator(allocator) {}
  virtual Result<nothing> run() = 0;
  void start() {
    tryCatch(run(), err, ({
               kernel::platform::impl<kernel::devices::SerialPort>::type serial;
               serial.initialize();
               format(
                   serial, "Uncaught error: ", err.name, ", CPU #",
                   kernel::platform::impl<kernel::platform::cpuid>::function(),
                   " terminated.\n");
               kernel::platform::impl<kernel::platform::halt>::function();
               nothing{};
             }));
  }
};

class BootstrapProcessor : public Processor {

  int initialized = false;
  u16 ncpus = 1;

public:
  constexpr BootstrapProcessor(kernel::pmm::Allocator &allocator)
      : Processor(allocator) {}
  BootstrapProcessor(BootstrapProcessor &) = delete;

  void setNumCPUs(int n_cpus) { ncpus = n_cpus; }

  virtual Result<nothing> run() {
    tryUnwrap(kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator));
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    tryUnwrap(serial.initialize());
    format(serial, "ParaOS\n");
    format(serial, "Available memory: ",
           this->allocator.availableMemory() / (1024 * 1024), "MB\n");
    format(serial, "Number of CPUs: ", ncpus, "\n");
    format(serial, "CPU #",
           kernel::platform::impl<kernel::platform::cpuid>::function(),
           " (BSP) ready\n");

    __atomic_store_n(&initialized, true, __ATOMIC_SEQ_CST);
    kernel::platform::impl<kernel::platform::halt>::function();
    return nothing{};
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

  virtual Result<nothing> run() {
    bsp.waitUntilInitialized();
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    tryUnwrap(serial.initialize());

    tryUnwrap(kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator));
    format(serial, "CPU #",
           kernel::platform::impl<kernel::platform::cpuid>::function(),
           " ready\n");
    kernel::platform::impl<kernel::platform::halt>::function();
    return nothing{};
  }
};

} // namespace kernel
