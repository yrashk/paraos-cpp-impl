#define tryUnwrap(e)                                                           \
  ({                                                                           \
    auto v = e;                                                                \
    auto value = v.result.value;                                               \
    if (!v.success)                                                            \
      return v.result.error;                                                   \
    value;                                                                     \
  })

#define tryCatch(e, err, c)                                                    \
  ({                                                                           \
    auto v = e;                                                                \
    v.success ? v.result.value : ({                                            \
      [[maybe_unused]] auto err = e.error();                                   \
      c;                                                                       \
    });                                                                        \
  })
