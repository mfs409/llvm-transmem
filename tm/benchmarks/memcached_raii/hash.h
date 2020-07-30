#pragma once

#include "../../common/tm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

TX_SAFE uint32_t hash(const void *key, size_t length, const uint32_t initval);

#ifdef __cplusplus
}
#endif
