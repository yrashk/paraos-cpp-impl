export module libpara.sync;

export namespace libpara::sync {

class Lock;

class LockGuard {
  friend class Lock;
  Lock &lock;
  LockGuard(Lock &lock);

public:
  ~LockGuard();
};

class Lock {
  bool locked = false;

public:
  Lock() : locked(false) {}
  Lock(bool locked) : locked(locked) {}

  void lock();

  LockGuard lockWithGuard();

  void unlock();
};

LockGuard::LockGuard(Lock &lock) : lock(lock) {}
LockGuard::~LockGuard() { lock.unlock(); }

void Lock::lock() {
  while (__atomic_exchange_n(&locked, true, __ATOMIC_SEQ_CST) == true) {
    __builtin_ia32_pause();
  }
}

LockGuard Lock::lockWithGuard() {
  lock();
  return LockGuard(*this);
}

void Lock::unlock() { __atomic_store_n(&locked, false, __ATOMIC_SEQ_CST); }
}; // namespace libpara::sync
