export module libpara.formatting;

import libpara.basic_types;
import libpara.concepts;

using namespace libpara::basic_types;
using namespace libpara;
using namespace libpara::concepts;

const constexpr char *digits = "0123456789";
const constexpr double log10_2 = 0.30102999566398;

export namespace libpara::formatting {

template <typename T>
concept writer = requires(T t, const char *s) {
  {t.write(s)};
};

template <writer W, typename T, typename... Ts> struct formatter {};

template <writer W, typename... Ts> struct formatter<W, const char *, Ts...> {
  static void format(W &writer, const char *value, Ts... rest) {
    writer.write(value);
    if constexpr (sizeof...(rest) > 0)
      formatter<W, Ts...>::format(writer, rest...);
  }
};

template <writer W, concepts::integer T, typename... Ts>
struct formatter<W, T, Ts...> {
  static void format(W &writer, T value, Ts... rest) {
    // For simplicity's sake, we allow log10(2) * bitwidth + 1
    // + 1 byte for null terminator + 1 (if signed) for sign
    const constexpr usize len = (log10_2 * sizeof(T) * 8) + 2 +
                                (concepts::is_signed_integer<T>::value ? 1 : 0);
    auto negative = false;
    if constexpr (concepts::is_signed_integer<T>::value) {
      if (value < 0) {
        value = -value;
        negative = true;
      }
    }
    char buf[len];
    buf[len - 1] = 0;
    auto i = len - 1;
    if (value == 0) {
      i--;
      buf[i] = '0';
    } else
      while (value > 0) {
        i--;
        buf[i] = (char)digits[value % 10];
        value = value / 10;
      }
    if (negative) {
      i--;
      buf[i] = '-';
    }
    writer.write(buf + i);

    if constexpr (sizeof...(rest) > 0)
      formatter<W, Ts...>::format(writer, rest...);
  }
};

template <writer W, typename T, typename... Ts>
void format(W &writer, T value, Ts... rest) {
  formatter<W, T, Ts...>::format(writer, value, rest...);
}

} // namespace libpara::formatting
