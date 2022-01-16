export module libpara.concepts;

import libpara.basic_types;

using namespace libpara::basic_types;

export namespace libpara::concepts {

template <typename T> struct is_integer {
  constexpr static bool value = false;
};

template <typename T> struct is_signed_integer {
  constexpr static bool value = false;
};

template <> struct is_integer<u8> { constexpr static bool value = true; };
template <> struct is_integer<u16> { constexpr static bool value = true; };
template <> struct is_integer<u32> { constexpr static bool value = true; };
template <> struct is_integer<u64> { constexpr static bool value = true; };
template <> struct is_integer<usize> { constexpr static bool value = true; };

template <> struct is_integer<i8> { constexpr static bool value = true; };
template <> struct is_integer<i16> { constexpr static bool value = true; };
template <> struct is_integer<i32> { constexpr static bool value = true; };
template <> struct is_integer<i64> { constexpr static bool value = true; };

template <> struct is_signed_integer<i8> {
  constexpr static bool value = true;
};
template <> struct is_signed_integer<i16> {
  constexpr static bool value = true;
};
template <> struct is_signed_integer<i32> {
  constexpr static bool value = true;
};
template <> struct is_signed_integer<i64> {
  constexpr static bool value = true;
};

template <typename T>
concept integer = is_integer<T>::value;

template <typename T>
concept signed_integer = is_signed_integer<T>::value;

} // namespace libpara::concepts
