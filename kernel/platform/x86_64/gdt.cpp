export module kernel.platform.x86_64.gdt;

import libpara.basic_types;

using namespace libpara::basic_types;

export namespace kernel::platform::x86_64::gdt {

enum Flag : u8 {
  Blocks4K = 1 << 3,
  Blocks1B = 0 << 3,
  LongMode = 1 << 1,
};

inline Flag operator|(Flag a, Flag b) {
  return static_cast<Flag>(static_cast<u8>(a) | static_cast<u8>(b));
}

enum Access : u8 {
  Present = 1 << 7,
  NotPresent = 0 << 7,
  Privilege0 = 0 << 4,
  Privilege1 = 1 << 4,
  Privilege2 = 2 << 4,
  Privilege3 = 3 << 4,
  Code = 1 << 4 | 1 << 3,
  Data = 1 << 4,
  Direction = 1 << 2,
  Conforming = 1 << 2,
  CodeReadable = 1 << 1,
  DataWritable = 1 << 1,
  Accessed = 1,
};

inline Access operator|(Access a, Access b) {
  return static_cast<Access>(static_cast<u8>(a) | static_cast<u8>(b));
}

class Segment {

  static const auto header_access = 5;
  static const auto header_flags = 6;

  u8 segment[8] = {0}; // 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

public:
  Segment() { setAccess(NotPresent); }
  Segment(Access access) { setAccess(access | Present); }
  Segment(Flag flags, Access access) {
    setFlags(flags);
    setAccess(access | Present);
  }

  void setFlags(Flag flags) {
    *reinterpret_cast<u8 *>(segment + header_flags) = flags << 4;
  }

  void setAccess(Access access) {
    *reinterpret_cast<Access *>(segment + header_access) = access;
  }

  Access getAccess() {
    return *reinterpret_cast<Access *>(segment + header_access);
  }
};

template <u16 n_segments> struct Register {
  u16 size = (1 + n_segments) * sizeof(Segment) - 1;
  Segment *offset = &null_segment;
  Segment null_segment = Segment();
  Segment segments[n_segments];

  void load() {
    asm volatile("lgdt %0" ::"m"(*this));

    u16 data_segment, code_segment;

    for (u16 i = 0; i < n_segments; i++) {
      if ((segments[i].getAccess() & (Privilege0 | Code)) ==
          (Privilege0 | Code))
        code_segment = i;
      if ((segments[i].getAccess() & (Privilege0 | Data)) ==
          (Privilege0 | Data))
        data_segment = i;
    }

    const u64 ds = (data_segment + 1) * sizeof(Segment);
    const u64 cs = (code_segment + 1) * sizeof(Segment);

    asm volatile("mov %0, %%ds ;  mov %0, %%fs ;  mov %0, %%gs "
                 "; mov %0, %%es ;  mov %0, %%ss"
                 :
                 : "r"(ds));

    asm volatile("push %0 ; lea 1f(%%rip), %%rax; push %%rax; .byte 0x48, "
                 "0xcb; 1: " ::"r"(cs)
                 : "rax");
  }

} __attribute__((packed));

} // namespace kernel::platform::x86_64::gdt

static_assert(sizeof(kernel::platform::x86_64::gdt::Segment) == 8);
