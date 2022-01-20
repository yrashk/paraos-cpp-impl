export module kernel.platform.x86_64.panic;

import libpara.basic_types;
import kernel.devices.serial;
import kernel.platform.x86_64.serial;

using namespace libpara::basic_types;

export namespace kernel::platform::x86_64::panic {

struct interrupt_frame {
  usize ip;
  usize cs;
  usize flags;
  usize sp;
  usize ss;
};

template <u64 interrupt> struct exception_name {
  static constexpr const char *name = "Unknown";
};

template <> struct exception_name<0x00> {
  static constexpr const char *name = "Divide by zero";
};

template <> struct exception_name<0x01> {
  static constexpr const char *name = "Debug";
};

template <> struct exception_name<0x02> {
  static constexpr const char *name = "Non-maskable interrupt";
};

template <> struct exception_name<0x03> {
  static constexpr const char *name = "Breakpoint";
};

template <> struct exception_name<0x04> {
  static constexpr const char *name = "Overflow";
};

template <> struct exception_name<0x05> {
  static constexpr const char *name = "Bound range exceeded";
};

template <> struct exception_name<0x06> {
  static constexpr const char *name = "Invalid opcode";
};

template <> struct exception_name<0x07> {
  static constexpr const char *name = "Device not available";
};

template <> struct exception_name<0x08> {
  static constexpr const char *name = "Double fault";
};

template <> struct exception_name<0x09> {
  static constexpr const char *name = "Coprocessor Segment Overrun";
};

template <> struct exception_name<0x0A> {
  static constexpr const char *name = "Invalid TSS";
};

template <> struct exception_name<0x0B> {
  static constexpr const char *name = "Segment Not Present";
};

template <> struct exception_name<0x0C> {
  static constexpr const char *name = "Stack-Segment Fault";
};

template <> struct exception_name<0x0D> {
  static constexpr const char *name = "General Protection Fault";
};

template <> struct exception_name<0x0E> {
  static constexpr const char *name = "Page Fault";
};

template <> struct exception_name<0x10> {
  static constexpr const char *name = "x87 Floating-Point Exception";
};

template <> struct exception_name<0x11> {
  static constexpr const char *name = "Alignment Check";
};

template <> struct exception_name<0x12> {
  static constexpr const char *name = "Machine Check";
};

template <> struct exception_name<0x13> {
  static constexpr const char *name = "SIMD Floating-Point Exception";
};

template <> struct exception_name<0x14> {
  static constexpr const char *name = "Virtualization Exception";
};

template <u64 interrupt> struct panic_isr {

  [[gnu::interrupt]] static void isr(interrupt_frame *frame, usize error_code) {
    auto serial = kernel::platform::x86_64::SerialPort();
    serial.initialize();
    serial.write("PANIC: ");
    serial.write(exception_name<interrupt>::name);
    serial.write("\n");
    asm volatile("cli ; hlt");
  }
};

} // namespace kernel::platform::x86_64::panic
