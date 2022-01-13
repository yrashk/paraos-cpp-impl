export module kernel.platform.x86_64.port;

import libpara.basic_types;

using namespace libpara::basic_types;

export namespace kernel::platform::x86_64 {

class Port {

  u16 port;

public:
  static const auto COM1 = 0x3F8;

  Port() : port(COM1) {}

  void out(u16 offset, u8 val) {
    u16 port_ = port + offset;
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port_));
  }

  u8 in(u16 offset) {
    u16 port_ = port + offset;
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port_));
    return ret;
  }
};

} // namespace kernel::platform::x86_64
