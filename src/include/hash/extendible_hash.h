/*
 * extendible_hash.h : implementation of in-memory hash table using extendible
 * hashing
 *
 * Functionality: The buffer pool manager must maintain a page table to be able
 * to quickly map a PageId to its corresponding memory location; or alternately
 * report that the PageId does not match any currently-buffered page.
 */

#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <mutex>

#include "hash/hash_table.h"

namespace cmudb {

template <typename K, typename V>
class ExtendibleHash : public HashTable<K, V> {
  struct Bucket{
    Bucket() = default;
    explicit Bucket(size_t _id, int _depth): id(_id), local_depth(_depth){}
    std::map<K, V> items;
    bool is_full = false;
    int id = 0;
    size_t local_depth = 0;
  };
public:
  // constructor
  ExtendibleHash(size_t size);
  // helper function to generate hash addressing
  size_t HashKey(const K &key);
  // helper function to get global & local depth
  int GetGlobalDepth() const;
  int GetLocalDepth(int bucket_id) const;
  int GetNumBuckets() const;
  // lookup and modifier
  bool Find(const K &key, V &value) override;
  bool Remove(const K &key) override;
  void Insert(const K &key, const V &value) override;

private:
  // add your own member variables here
  int global_depth; // global depth
  mutable std::mutex mtx; // protects the hash table.

  const size_t bucket_size; // maximum number of entries in a bucket
  size_t kv_count; // key-value pairs
  std::vector<std::shared_ptr<Bucket>> hashmap;  
};
} // namespace cmudb
