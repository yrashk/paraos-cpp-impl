export module kernel.platform;

import libpara.basic_types;

using namespace libpara::basic_types;

export namespace kernel::platform {

template <typename T>
concept Platform = requires(T) {
  true;
};

struct X86_64 {};

#ifndef PARAOS_TARGET
#define PARAOS_TARGET X86_64
#endif

using Target = PARAOS_TARGET;

template <typename Iface, Platform P = Target> struct impl;

/**
 * Gets current CPU's ID
 */
using cpuid = u16();

/**
 * Halts the CPU
 */
using halt = void();

} // namespace kernel::platform
