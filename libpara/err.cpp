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
   * Initialize as a success
   */
  Result(T value) : result{.value = value}, success(true) {}
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
      Assert(err.name[0] == 't');
      Assert(err.name[1] == 'e');
      Assert(err.name[2] == 's');
      Assert(err.name[3] == 't');
      Assert(err.name[4] == 0);
    }

    test("Error equality");
    {

      auto err1 = Error("test1");
      auto err2 = Error("test2");
      auto err3 = Error("test1");

      Assert(err1 == err3);
      Assert(err1 != err2);
    }
    test("Result construction");
    {

      Result<int> r(1);
      Assert(r.success);
      Assert(r == 1);
      Assert(*r == 1);

      Result<int> r1(Error("e"));
      Assert(!r1.success);
      Assert(r1 == Error("e"));
      Assert(r1.error() == Error("e"));
    }

    test("return_error");
    {
      auto fret = []() -> Result<int> {
        return return_error(Error("failure"));
      };
      Assert(fret() == Error("failure"));
      auto fret_str = []() -> Result<int> { return return_error("failure"); };
      Assert(fret_str() == Error("failure"));
      auto fret_unsup = []() -> Result<int> { return return_error(true); };
      Assert(fret_unsup() == Error("Unsupported error"));
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
      Assert(f_try() == 2);
      Assert(f_try_fail() == Error("err"));
    }

    test("tryCatch");
    {
      Result<int> r(Error("err"));
      Assert(tryCatch(r, 1) == 1);
      Result<int> r1(10);
      Assert(tryCatch(r1, 1) == 10);
    }
  }
};
} // namespace libpara::err::tests

