export module libpara.err;

import libpara.xxh64;
import libpara.testing;

export namespace libpara::err {

struct Error;
template <typename T> struct Result;

template <typename T> consteval const Error return_error(T error);

/**
 * Named error with no further details
 */
struct Error {
  // name
  const char *name;
  // unique error id
  const unsigned long long id;

  /**
   * Constructs a error
   *
   * Every error gets a unique ID based on xxHash64 of its name
   * https://github.com/Cyan4973/xxHash
   * At this moment we assume that the collision properties of this hash
   * are decent:
   * https://github.com/Cyan4973/xxHash/tree/dev/tests/collisions#examples
   *
   */
  consteval Error(const char *name)
      : name(name), id(libpara::xxh64::hash(name, 0)) {}

  /**
   *  Can compare with other Errors
   */
  constexpr bool operator==(const Error &err) const { return id == err.id; }

  /**
   * Can compare with Results. True only if result has not succeeded and error
   * matches
   */
  template <typename T> constexpr bool operator==(const Result<T> &res) const {
    return !res.success && res.result.error == *this;
  }
};

/**
 * Handles return of an Error
 */
template <> consteval const Error return_error(Error error) { return error; }
/**
 * Handles return of a const string as an Error
 */
template <> consteval const Error return_error(const char *name) {
  return Error(name);
}

/**
 * Handles an overloaded return of an error, handling unsupported
 * error type T
 */
template <typename T> consteval const Error return_error(T error) {
  return Error("Unsupported error");
}

/**
 * Result is a union of a "success" value or an error, tagged with
 * the success
 */
template <typename T> struct Result {
  union {
    T value;
    Error error;
  } result;
  bool success;

  /**
   * Initialize as a success from an lvalue
   */
  Result(T &value) : result{.value = value}, success(true) {}
  /**
   * Initialize as a success from an rvalue
   */
  Result(T &&value) : result{.value = static_cast<T>(value)}, success(true) {}

  /**
   * Initialize as an error
   */
  Result(Error error) : result{.error = error}, success(false) {}

  /**
   * Get result if success, undefined if it isn't
   */
  constexpr const T *operator->() const { return &result.value; }

  /**
   * Get result if success, undefined if it isn't
   */
  constexpr const T &operator*() const & { return result.value; }

  /**
   * Compare with a value. True if successful and value is equal
   */
  constexpr bool operator==(T &&value) {
    return success && result.value == value;
  }

  /**
   * Get error if not success, undefined if it is not an error
   */
  constexpr Error error() const { return result.error; }
};

} // namespace libpara::err

#include <err.hpp>
#include <testing.hpp>

export namespace libpara::err::tests {

class TestCase : public libpara::testing::TestCase {

public:
  using libpara::testing::TestCase::TestCase;

  virtual void run() {
    test("Error construction");
    {

      auto err = Error("test");
      Expect(err.name[0] == 't');
      Expect(err.name[1] == 'e');
      Expect(err.name[2] == 's');
      Expect(err.name[3] == 't');
      Expect(err.name[4] == 0);
    }

    test("Error equality");
    {

      auto err1 = Error("test1");
      auto err2 = Error("test2");
      auto err3 = Error("test1");

      Expect(err1 == err3);
      Expect(err1 != err2);
    }
    test("Result construction");
    {

      Result<int> r(1);
      Expect(r.success);
      Expect(r == 1);
      Expect(*r == 1);

      Result<int> r1(Error("e"));
      Expect(!r1.success);
      Expect(r1 == Error("e"));
      Expect(r1.error() == Error("e"));

      int val = 1;
      Result<int> r2(val);
      Expect(r2.success);
      Expect(r2 == 1);
      Expect(*r2 == 1);
    }

    test("return_error");
    {
      auto fret = []() -> Result<int> {
        return return_error(Error("failure"));
      };
      Expect(fret() == Error("failure"));
      auto fret_str = []() -> Result<int> { return return_error("failure"); };
      Expect(fret_str() == Error("failure"));
      auto fret_unsup = []() -> Result<int> { return return_error(true); };
      Expect(fret_unsup() == Error("Unsupported error"));
    }
    test("tryUnwrap");
    {
      auto f_fail = []() -> Result<int> { return Error("err"); };
      auto f = []() -> Result<int> { return 1; };
      auto f_try = [=]() -> Result<int> {
        tryUnwrap(f());
        return 2;
      };
      auto f_try_fail = [=]() -> Result<int> {
        tryUnwrap(f_fail());
        return 1;
      };
      Expect(f_try() == 2);
      Expect(f_try_fail() == Error("err"));
    }

    test("tryUnwrap executes code block once");
    {
      int i = 0;
      auto f = [&]() -> Result<int> {
        i += 1;
        return 1;
      };
      auto f_try = [&]() -> Result<int> { return tryUnwrap(f()); };
      Expect(f_try() == 1);
      Expect(i == 1);

      i = 0;

      auto f_fail = [&]() -> Result<int> {
        i += 1;
        return Error("err");
      };

      auto f_fail_try = [&]() -> Result<int> { return tryUnwrap(f_fail()); };
      Expect(f_fail_try() == Error("err"));
      Expect(i == 1);
    }

    test("tryCatch");
    {
      Result<int> r(Error("err"));
      Expect(tryCatch(r, 1) == 1);
      Result<int> r1(10);
      Expect(tryCatch(r1, 1) == 10);
    }
  }
};
} // namespace libpara::err::tests

