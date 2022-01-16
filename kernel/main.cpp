export module kernel.main;

import kernel.devices.serial;
#ifndef RELEASE
import kernel.testing;
import libpara.testing;
import libpara.err;
#endif
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
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    serial.initialize();
    format(serial, "ParaOS\n");
    format(serial, "Available memory: ",
           this->allocator.availableMemory() / (1024 * 1024), "MB\n");
#ifndef RELEASE
    serial.write("Self-testing: ");
    {
      // destruct sink at the end of the block
      auto sink = kernel::testing::SerialConsoleSink(serial);

      libpara::err::tests::TestCase(sink).start();
      kernel::pmm::tests::TestCase(sink).start();
    }
    serial.write("Self-testing done.\n");
#endif
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

template <pmm::allocator A> class ApplicationProcessor : public Processor<A> {

  BootstrapProcessor<A> bsp;

public:
  ApplicationProcessor(A allocator, BootstrapProcessor<A> &bsp)
      : Processor<A>(allocator), bsp(bsp) {}

  virtual void run() {
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

} // namespace kernel
