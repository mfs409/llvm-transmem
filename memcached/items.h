#include "../include/tm.h"
/* See items.c */
TX_SAFE uint64_t get_cas_id(void);

/*@null@*/
TX_SAFE item *do_item_alloc(char *key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes, const uint32_t cur_hv);
TX_SAFE void item_free(item *it);
bool item_size_ok(const size_t nkey, const int flags, const int nbytes);

TX_SAFE int  do_item_link(item *it, const uint32_t hv);     /** may fail if transgresses limits */
TX_SAFE void do_item_unlink(item *it, const uint32_t hv);
TX_SAFE void do_item_unlink_nolock(item *it, const uint32_t hv);
TX_SAFE void do_item_remove(item *it);
TX_SAFE void do_item_update(item *it);   /** update LRU time to current and reposition */
TX_SAFE int  do_item_replace(item *it, item *new_it, const uint32_t hv);

/*@null@*/
TX_SAFE char *do_item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
TX_SAFE void do_item_stats(ADD_STAT add_stats, void *c);
TX_SAFE void do_item_stats_totals(ADD_STAT add_stats, void *c);
/*@null@*/
TX_SAFE void do_item_stats_sizes(ADD_STAT add_stats, void *c);
TX_SAFE void do_item_flush_expired(void);

TX_SAFE item *do_item_get(const char *key, const size_t nkey, const uint32_t hv);
TX_SAFE item *do_item_touch(const char *key, const size_t nkey, uint32_t exptime, const uint32_t hv);
void item_stats_reset(void);
extern pthread_mutex_t cache_lock;
TX_SAFE void item_stats_evictions(uint64_t *evicted);
