export module kernel.main;

import kernel.devices.serial;
#ifndef RELEASE
import kernel.testing;
import libpara.testing;
import libpara.err;
#endif
import kernel.platform;
import kernel.platform.x86_64;
import kernel.platform.x86_64.serial;

export namespace kernel {

class Processor {

public:
  Processor() {}
  virtual void run() = 0;
};

class BootstrapProcessor : public Processor {

public:
  BootstrapProcessor() {}

  virtual void run() {
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    serial.initialize();
    serial.write("ParaOS\n");
#ifndef RELEASE
    serial.write("Self-testing: ");
    {
      // destruct sink at the end of the block
      auto sink = kernel::testing::SerialConsoleSink(serial);

      libpara::err::tests::TestCase(sink).start();
    }
    serial.write("Self-testing done.\n");
#endif
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

class ApplicationProcessor : public Processor {

  BootstrapProcessor &bsp;

public:
  ApplicationProcessor(BootstrapProcessor &bsp) : bsp(bsp) {}

  virtual void run() {
    kernel::platform::impl<kernel::platform::halt>::function();
  }
};

} // namespace kernel
