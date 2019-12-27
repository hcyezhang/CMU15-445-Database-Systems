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
#include <map>

#include "hash/hash_table.h"

namespace cmudb {

template <typename K, typename V>
class ExtendibleHash : public HashTable<K, V> {
  struct Bucket{
    Bucket() = default;
    explicit Bucket(size_t _id, int _depth): id(_id), local_depth(_depth){}
    std::map<K, V> items;
    bool overflow = false;
    size_t id = 0;
    size_t local_depth = 0;
  };
public:
  // constructor
  explicit ExtendibleHash(size_t size);

  ExtendibleHash(const ExtendibleHash &) = delete;
  ExtendibleHash &operator=(const ExtendibleHash &) = delete;
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

  size_t Size() const override {return pair_count;}

private:
  // add your own member variables here
  size_t global_depth; // global depth
  mutable std::mutex mtx; // protects the hash table.

  const size_t bucket_size; // maximum number of entries in a bucket
  int bucket_count; // number of buckets in use.
  size_t pair_count; // key-value pairs
  std::vector<std::shared_ptr<Bucket>> hashmap;  

  std::unique_ptr<Bucket> split(std::shared_ptr<Bucket> &);

  // helper function to get the index of the bucket a key belongs to
  size_t GetBucketIndex(const K &key);
};
} // namespace cmudb
