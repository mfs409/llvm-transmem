#ifndef HASH_H
#define    HASH_H

#include "../include/tm.h"

#ifdef    __cplusplus
extern "C" {
#endif

TX_SAFE uint32_t hash(const void *key, size_t length, const uint32_t initval);

#ifdef    __cplusplus
}
#endif

#endif    /* HASH_H */

