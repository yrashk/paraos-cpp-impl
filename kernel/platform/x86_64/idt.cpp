export module kernel.platform.x86_64.idt;

import libpara.basic_types;

using namespace libpara::basic_types;

export namespace kernel::platform::x86_64::idt {

enum GateType : u8 { Interrupt = 0x0E, Trap = 0x0F };

class Gate {
  static const auto header_offset_low = 0;
  static const auto header_offset_mid = 6;
  static const auto header_offset_high = 8;
  static const auto header_segment = 2;
  static const auto header_ist = 4;
  static const auto header_attrs = 5;

  u8 descriptor[16] = {0};

public:
  Gate() { setPresent(false); }

  Gate(void *ptr, u16 segment, GateType type, u8 dpl = 0, u8 ist = 0) {
    setPointer(ptr);
    setSegment(segment);
    setType(type);
    setDPL(dpl);
    setPresent(true);
  }

  void setPointer(void *ptr) {
    u64 offset = reinterpret_cast<u64>(ptr);
    *reinterpret_cast<u16 *>(descriptor + header_offset_low) =
        static_cast<u16>(offset);
    *reinterpret_cast<u16 *>(descriptor + header_offset_mid) =
        static_cast<u16>(offset >> 16);
    *reinterpret_cast<u32 *>(descriptor + header_offset_high) =
        static_cast<u32>(offset >> 32);
  }

  void setSegment(u16 segment) {
    *reinterpret_cast<u16 *>(descriptor + header_segment) = segment;
  }

  void setType(GateType type) {
    auto attrs = reinterpret_cast<u8 *>(descriptor + header_attrs);
    *attrs = *attrs | (type & 0x0f);
  }

  void setDPL(u8 dpl) {
    auto attrs = reinterpret_cast<u8 *>(descriptor + header_attrs);
    *attrs = *attrs | (dpl & 0x0f) << 4;
  }

  void setPresent(bool present) {
    auto attrs = reinterpret_cast<u8 *>(descriptor + header_attrs);
    *attrs = *attrs | (present ? 1 : 0) << 7;
  }
};

template <int n = 256> struct Register {
  u16 size = n * sizeof(Gate) - 1;
  Gate *offset = gates;
  Gate gates[n] = {Gate()};

  void load() { asm volatile("lidt %0" ::"m"(*this)); }
} __attribute__((packed));

} // namespace kernel::platform::x86_64::idt

static_assert(sizeof(kernel::platform::x86_64::idt::Gate) == 16);
