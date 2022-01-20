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
 * Initializes the platform
 */
struct initialize {};

/**
 * Gets current CPU's ID
 */
struct cpuid {};

/**
 * Halts the CPU
 */
struct halt {};

/**
 * Terminates the emulator
 */
struct exit_emulator {};

} // namespace kernel::platform
