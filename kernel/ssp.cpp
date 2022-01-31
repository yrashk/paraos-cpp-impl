export module kernel.ssp;

// Stack Smashing Protector

import libpara.basic_types;
import libpara.formatting;

import kernel.devices.serial;
import kernel.platform;
import kernel.platform.x86_64;

using namespace libpara::basic_types;
using namespace libpara::formatting;

export constinit __attribute__((used)) usize __stack_chk_guard =
    sizeof(usize) == 8 ? 0x595e9fbd94fda766 : 0xe2dee396;

export extern "C" __attribute__((noreturn)) void __stack_chk_fail(void) {
  kernel::platform::impl<kernel::devices::SerialPort>::type serial;
  serial.initialize();
  format(serial, "Stack smashed on CPU #",
         kernel::platform::impl<kernel::platform::cpuid>::function(), "\n");
  kernel::platform::impl<kernel::platform::halt>::function();
  while (true) {
  }
}
