export module kernel.testing;

import libpara.basic_types;
import libpara.testing;
import libpara.err;
import libpara.loop;
import kernel.pmm;
import kernel.devices.serial;
import kernel.platform;
import kernel.platform.x86_64;

using namespace libpara::basic_types;

export namespace kernel::testing {

class SerialConsoleSink : public libpara::testing::TestCaseSink {

  using serial_port_t =
      kernel::platform::impl<kernel::devices::SerialPort>::type;

  serial_port_t &serial_port;
  const char *in_test = nullptr;
  bool errored = false;

  int errors = 0;

public:
  SerialConsoleSink(serial_port_t &serial_port) : serial_port(serial_port) {}
  ~SerialConsoleSink() { serial_port.write("\n"); }

  virtual void test(const char *name) {
    testComplete();
    in_test = name;
    errored = false;
  }

  virtual void testComplete() {
    if (in_test != nullptr) {
      if (!errored)
        serial_port.write(".");
      in_test = nullptr;
      errored = false;
    }
  }

  virtual void report(bool success, const char *message = "",
                      const char *file = nullptr, const char *line = nullptr) {
    if (!success) {
      errors++;
      if (!errored) {
        errored = true;
      }
      serial_port.write("\n[!] Failure: ");
      serial_port.write(in_test);
      serial_port.write(": ");
      serial_port.write(message);
      serial_port.write(" at ");
      if (file != nullptr) {
        serial_port.write(file);
        if (line != nullptr) {
          serial_port.write(":");
          serial_port.write(line);
        }
      }
      serial_port.write("\n");
    }
  };

  virtual void write(const char *s) { serial_port.write(s); }

  bool hadAnyErrors() const { return errors > 0; }
};

inline void run() {
  bool hadAnyErrors = false;
  {
    kernel::platform::impl<kernel::devices::SerialPort>::type serial;
    serial.initialize();
    auto sink = SerialConsoleSink(serial);

    libpara::err::tests::TestCase(sink).start();
    libpara::loop::tests::TestCase(sink).start();
    kernel::pmm::tests::TestCase(sink).start();
    hadAnyErrors = sink.hadAnyErrors();
  }
  kernel::platform::impl<kernel::platform::exit_emulator>::function(
      hadAnyErrors ? 1 : 0);
}

} // namespace kernel::testing

