#pragma once

#define tryUnwrap(e)                                                           \
  ({                                                                           \
    auto v = e;                                                                \
    auto value = v.result.value;                                               \
    if (!v.success)                                                            \
      return v.result.error;                                                   \
    value;                                                                     \
  })

#define tryCatch(e, c) ({ e.success ? e.result.value : c; })
