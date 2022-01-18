export module libpara.basic_types;

export namespace libpara::basic_types {

using nullptr_t = decltype(nullptr);

using u8 = unsigned char;
using i8 = char;

using u16 = unsigned short;
using i16 = short;

using u32 = unsigned int;
using i32 = int;

using u64 = unsigned long long;
using i64 = long long;

using usize = decltype(sizeof(nullptr));

struct nothing {
  u8 nothing[0];
};

} // namespace libpara::basic_types

using namespace libpara::basic_types;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(i8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(i64) == 8);
static_assert(sizeof(usize) == sizeof(nullptr));
static_assert(sizeof(nothing) == 0);
