export module kernel.testing;

import libpara.testing;
import kernel.devices.serial;
import kernel.platform;
import kernel.platform.x86_64.serial;

export namespace kernel::testing {

class SerialConsoleSink : public libpara::testing::TestCaseSink {

  using serial_port_t =
      kernel::platform::impl<kernel::devices::SerialPort>::type;

  serial_port_t &serial_port;
  const char *in_test = nullptr;
  bool errored = false;

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
};

} // namespace kernel::testing

