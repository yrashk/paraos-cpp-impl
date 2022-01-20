export module kernel.main;

import kernel.devices.serial;
import libpara.concepts;
import libpara.formatting;
import kernel.pmm;
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;

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

public:
  BootstrapProcessor(kernel::pmm::Allocator &allocator)
      : Processor(allocator) {}

  virtual void run() {
    kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator);
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    serial.initialize();
    format(serial, "ParaOS\n");
    format(serial, "Available memory: ",
           this->allocator.availableMemory() / (1024 * 1024), "MB\n");
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

class ApplicationProcessor : public Processor {

  BootstrapProcessor bsp;

public:
  ApplicationProcessor(kernel::pmm::Allocator &allocator,
                       BootstrapProcessor &bsp)
      : Processor(allocator), bsp(bsp) {}

  virtual void run() {
    kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator);
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

} // namespace kernel
