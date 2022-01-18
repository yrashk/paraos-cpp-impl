export module libpara.loop;

import libpara.concepts;

using namespace libpara::concepts;

export namespace libpara::loop {

template <integer I, I end, I start = 0, typename F>
constexpr void constexpr_loop(F &&f) {
  if constexpr (start < end) {
    f.template operator()<start>();
    constexpr_loop<I, end, start + 1>(f);
  }
}

} // namespace libpara::loop

import libpara.testing;

#include <testing.hpp>

export namespace libpara::loop::tests {

class TestCase : public libpara::testing::TestCase {

public:
  using libpara::testing::TestCase::TestCase;

  virtual void run() {
    test("constexpr_loop");
    {
      int a = 0;
      constexpr_loop<int, 10>([&]<int i>() { a += i; });
      Assert(a == 0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9);
    }
  };
};
} // namespace libpara::loop::tests
