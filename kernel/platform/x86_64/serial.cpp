export module kernel.platform.x86_64.serial;

import libpara.err;
import libpara.basic_types;

import kernel.devices.serial;
import kernel.platform;
import kernel.platform.x86_64.port;

using namespace libpara::err;
using namespace libpara::basic_types;

export namespace kernel::platform::x86_64 {

class SerialPort : public kernel::devices::SerialPort {
  Port port;

public:
  using kernel::devices::SerialPort::write;

  SerialPort() {}
  virtual Result<nullptr_t> initialize() {
    port.out(1, 0x00); // Disable all interrupts
    port.out(3, 0x80); // Enable DLAB (set baud rate divisor)
    port.out(0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    port.out(1, 0x00); //                  (hi byte)
    port.out(3, 0x03); // 8 bits, no parity, one stop bit
    port.out(2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    port.out(4, 0x0B); // IRQs enabled, RTS/DSR set
    port.out(4, 0x1E); // Set in loopback mode, test the serial chip
    port.out(0, 0xAE); // Test serial chip (send byte 0xAE and check if serial
                       // returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (port.in(0) != 0xAE)
      return return_error("FaultySerialPort");

    port.out(4, 0x0F);
    return nullptr;
  }

  virtual void write(const u8 b) {
    while (!isTransmitEmpty()) {
    }
    port.out(0, b);
  }

private:
  bool isTransmitEmpty() { return (port.in(5) & 0x20) != 0; }
};

} // namespace kernel::platform::x86_64

export namespace kernel::platform {

template <> struct impl<kernel::devices::SerialPort, kernel::platform::X86_64> {
  using type = kernel::platform::x86_64::SerialPort;
};

} // namespace kernel::platform

