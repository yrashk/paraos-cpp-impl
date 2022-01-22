export module libpara.sync;

export namespace libpara::sync {

class Lock {
  bool locked = false;

public:
  constexpr Lock() : locked(false) {}
  constexpr Lock(bool locked) : locked(locked) {}

  void lock() {
    while (__atomic_exchange_n(&locked, true, __ATOMIC_SEQ_CST) == true) {
      __builtin_ia32_pause();
    }
  }

  void unlock() { __atomic_store_n(&locked, false, __ATOMIC_SEQ_CST); }
};

class LockGuard {
  Lock &lock;

public:
  LockGuard(Lock &lock) : lock(lock) {}
  ~LockGuard() { lock.unlock(); }
};

}; // namespace libpara::sync
