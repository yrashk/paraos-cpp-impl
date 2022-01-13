export module libpara.basic_types;

export namespace libpara::basic_types {

using nullptr_t = decltype(nullptr);

using u8 = unsigned char;

using u16 = unsigned short;

using u32 = unsigned int;

using u64 = unsigned long long;

} // namespace libpara::basic_types

using namespace libpara::basic_types;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);
