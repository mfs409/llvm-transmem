#ifndef PTM_HASH_TABLE_H
#define PTM_HASH_TABLE_H

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "../include/tm.h"

#define ARRAY_SIZE 8

struct HashtableItem{
  uint64_t _keys[ARRAY_SIZE];       
  uint64_t _value[ARRAY_SIZE];
  size_t _top;
  HashtableItem() : _top(0) {
  }
}__attribute__((aligned(32)));

struct Hashtable{
  uint64_t size;
  HashtableItem* items;
}__attribute__((aligned(32)));

const uint64_t kFNVOffsetBasis64 = 0xCBF29CE484222325;
const uint64_t kFNVPrime64 = 1099511628211;

// hash function
inline uint64_t FNVHash64(uint64_t val) {
  uint64_t hash = kFNVOffsetBasis64;
  int i;
  for (i = 0; i < 8; i++) {
    uint64_t octet = val & 0x00ff;
    val = val >> 8;
    
    hash = hash ^ octet;
    hash = hash * kFNVPrime64;
  }
  return hash;
}

Hashtable* hash_table_init(uint64_t size){
  Hashtable *hashtable = (Hashtable*)aligned_alloc(32, sizeof(Hashtable));
  HashtableItem* items = (HashtableItem *)aligned_alloc(32, sizeof(HashtableItem) * size);
  if (items == nullptr || hashtable == nullptr)
      assert(false);
  hashtable->items = items;
  hashtable->size = size;
  return hashtable;
}

// This function should be called inside of transaction
TX_SAFE int hash_table_put(Hashtable* hashtable, uint64_t _key, uint64_t _value){
  uint64_t key = FNVHash64(_key);
  uint64_t i = key % hashtable->size;

  HashtableItem* item = &hashtable->items[i];
  if (item->_top == ARRAY_SIZE) { return -1; }
  item->_keys[item->_top] = _key;
  item->_value[item->_top] = _value;
  item->_top = item->_top + 1;
  return 0;
}

TX_SAFE uint64_t hash_table_read(struct Hashtable* hashtable, uint64_t _key){
  uint64_t key = FNVHash64(_key);
  uint64_t i = key % hashtable->size;
  
  HashtableItem* item = &hashtable->items[i];
  for (int l = 0; l < item->_top; l++) {
    if (item->_keys[l] == _key) return item->_value[l];
  }
  return 0;
}

long hash_table_account(Hashtable* hashtable){
  return 0;
}
#endif
