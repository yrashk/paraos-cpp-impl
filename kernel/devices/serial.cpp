export module kernel.devices.serial;

import libpara.err;
import libpara.basic_types;

using namespace libpara::err;
using namespace libpara::basic_types;

export namespace kernel::devices {

class SerialPort {

public:
  virtual Result<nullptr_t> initialize() = 0;
  virtual void write(const u8 b) = 0;

  virtual void write(const char *str) {
    auto i = 0;
    while (str[i] != 0) {
      write((u8)str[i]);
      i++;
    }
  }
};

} // namespace kernel::devices
