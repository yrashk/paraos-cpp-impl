export module kernel.main;

import kernel.devices.serial;
import libpara.formatting;
import kernel.pmm;
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;

using namespace libpara::formatting;

export namespace kernel {

template <pmm::allocator A> class Processor {
protected:
  A &allocator;

public:
  Processor(A &allocator) : allocator(allocator) {}
  virtual void run() = 0;
};

template <pmm::allocator A> class BootstrapProcessor : public Processor<A> {

public:
  BootstrapProcessor(A &allocator) : Processor<A>(allocator) {}

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

template <pmm::allocator A> class ApplicationProcessor : public Processor<A> {

  BootstrapProcessor<A> bsp;

public:
  ApplicationProcessor(A allocator, BootstrapProcessor<A> &bsp)
      : Processor<A>(allocator), bsp(bsp) {}

  virtual void run() {
    kernel::platform::impl<kernel::platform::initialize>::function(
        this->allocator);
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

} // namespace kernel
