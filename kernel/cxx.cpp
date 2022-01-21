extern "C" void __cxa_pure_virtual() {}

__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" int __cxa_guard_acquire(__guard *g) {
  return __atomic_exchange_n(g, 1, __ATOMIC_SEQ_CST) == 0;
}

extern "C" void __cxa_guard_release(__guard *g) {
  __atomic_store_n(g, 2, __ATOMIC_SEQ_CST);
}

extern "C" void __cxa_guard_abort(__guard *) {}
