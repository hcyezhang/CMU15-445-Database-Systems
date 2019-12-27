#include <list>

#include "hash/extendible_hash.h"
#include "page/page.h"
#include <vector>

namespace cmudb {

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size): 
  bucket_size(size),  bucket_count(0), pair_count(0){
  hashmap.emplace_back(new Bucket(0, 0));
  global_depth = 0;
  bucket_count = 1;
}

/*
 * helper function to calculate the hashing address of input key
 */
template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) {
  return std::hash<K>()(key);
}

/*
 * helper function to get the bucket index 
*/
template <typename K, typename V>
size_t ExtendibleHash<K, V>::GetBucketIndex(const K &key){
  return HashKey(key) & (( 1 << global_depth) -1 );
}

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetGlobalDepth() const {
  std::lock_guard<std::mutex> lock(mtx);
  return global_depth;
}

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
  std::lock_guard<std::mutex> lock(mtx);
  if (hashmap[bucket_id]){
    return hashmap[bucket_id]->local_depth;
  }
  return -1;
}

/*
 * helper function to return current number of bucket in hash table
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const {
  std::lock_guard<std::mutex> lock(mtx);
  return bucket_count;
}

/*
 * lookup function to find value associate with input key
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
  std::lock_guard<std::mutex> lock(mtx);
  size_t idx = GetBucketIndex(key);
  if(hashmap[idx]){
    auto bucket = hashmap[idx];
    if(bucket->items.count(key)){
      value = bucket->items[key];
      return true;
    }
  }
  return false;
}

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {
  std::lock_guard<std::mutex> lock(mtx);
  size_t num_of_pairs = 0;
  size_t idx = GetBucketIndex(key);
  
  if(hashmap[idx]){
    auto bucket = hashmap[idx];
    num_of_pairs += bucket->items.erase(key);
    pair_count -= num_of_pairs;
  }
  return num_of_pairs != 0;
}

/*
 * Helper function 
 */

template <typename K, typename V>
std::unique_ptr<typename ExtendibleHash<K,V>::Bucket>
ExtendibleHash<K,V>::split(std::shared_ptr<Bucket> &b){
  auto res = std::make_unique<Bucket>(0, b->local_depth);
  while (res->items.empty()){
    ++b->local_depth;
    ++res->local_depth;
    for(auto it = b->items.begin(); it != b->items.end();){
      if(HashKey(it->first) & ((1 << b->local_depth) - 1)){
        res->items.insert(*it);
        res->id = HashKey(it->first) &((1 << b->local_depth) - 1);
        it = b->items.erase(it);
      }else{
        ++it;
      }
    }
    if(b->items.empty()){
      b->items.swap(res->items);
      b->id = res->id;
    }
    if (b->local_depth == sizeof(size_t)*8){
      b->overflow = true;
      return nullptr;
    }
  }
  ++bucket_count;
  return res;
}
/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
  std::lock_guard<std::mutex> lock(mtx);
  size_t idx = GetBucketIndex(key);
  assert (idx < hashmap.size());
  if(hashmap[idx] == nullptr){
    hashmap[idx] = std::make_shared<Bucket>(idx, global_depth);
    bucket_count++;
  }

  auto bucket = hashmap[idx];
  if (bucket->items.count(key)){
    bucket->items[key] = value;
    return;
  }
  bucket->items.insert({key, value});
  pair_count++;

  if (bucket->items.size() > bucket_size && !bucket->overflow ){
    auto old_idx = bucket->id;
    auto old_depth = bucket->local_depth;
    std::shared_ptr<Bucket> new_bucket = split(bucket);
    
    if(new_bucket == nullptr){
      bucket->local_depth = old_depth;
      return;
    }

    if(bucket->local_depth > global_depth){
      auto sz = hashmap.size();
      auto factor = (1 << (bucket->local_depth - global_depth));

      global_depth = bucket->local_depth;
      hashmap.resize(hashmap.size() * factor);
      
      hashmap[bucket->id] = bucket;
      hashmap[new_bucket->id] = new_bucket;

      for(size_t i = 0; i < sz; i++){
        if(hashmap[i]){
          if (i < hashmap[i]->id ){
            hashmap[i].reset();
          }else{
            auto step = 1 << hashmap[i]->local_depth;
            for(size_t j = i+step; j < hashmap.size(); j += step){
              hashmap[j] = hashmap[i];
            }
          }
        }
      }
    } else{
      for (size_t i = old_idx; i < hashmap.size(); i += (1 << old_depth)){
        hashmap[i].reset();
      }
      hashmap[bucket->id] = bucket;
      hashmap[new_bucket->id] = new_bucket;

      auto step = 1 << bucket->local_depth;

      for(size_t i = bucket->id + step; i < hashmap.size(); i += step){
        hashmap[i] = bucket;
      }
      for(size_t i = new_bucket->id + step; i < hashmap.size(); i += step){
        hashmap[i] = new_bucket;
      }
    }
  }
  
}

template class ExtendibleHash<page_id_t, Page *>;
template class ExtendibleHash<Page *, std::list<Page *>::iterator>;
// test purpose
template class ExtendibleHash<int, std::string>;
template class ExtendibleHash<int, std::list<int>::iterator>;
template class ExtendibleHash<int, int>;
} // namespace cmudb
