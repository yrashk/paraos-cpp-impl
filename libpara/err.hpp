#pragma once

#define tryUnwrap(e)                                                           \
  ({                                                                           \
    auto result = e.result.value;                                              \
    if (!e.success)                                                            \
      return e.result.error;                                                   \
    result;                                                                    \
  })

#define tryCatch(e, c) ({ e.success ? e.result.value : c; })
