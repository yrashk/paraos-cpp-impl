export module libpara.testing;

import libpara.formatting;
using namespace libpara::formatting;

export namespace libpara::testing {

class TestCaseSink {

public:
  virtual void test(const char *name){};
  virtual void testComplete(){};
  virtual void report(bool success, const char *message = "",
                      const char *file = nullptr, const char *line = nullptr) {}
  virtual void write(const char *s) {}
};

class TestCase {

  TestCaseSink &sink;

public:
  TestCase(TestCaseSink &sink) : sink(sink) {}

  void start() {
    run();
    sink.testComplete();
  }

  virtual void run() = 0;

  void test(const char *name) { sink.test(name); }

  void assert(bool success, const char *message = "",
              const char *file = nullptr, const char *line = nullptr) {
    sink.report(success, message, file, line);
  }

  template <typename... Ts> void print(Ts... args) { format(sink, args...); }
  template <typename... Ts> void println(Ts... args) {
    format(sink, args..., "\n");
  }
};

} // namespace libpara::testing
