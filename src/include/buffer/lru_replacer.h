/**
 * lru_replacer.h
 *
 * Functionality: The buffer pool manager must maintain a LRU list to collect
 * all the pages that are unpinned and ready to be swapped. The simplest way to
 * implement LRU is a FIFO queue, but remember to dequeue or enqueue pages when
 * a page changes from unpinned to pinned, or vice-versa.
 */

#pragma once

#include <mutex>
#include <unordered_map>

#include "buffer/replacer.h"
#include "hash/extendible_hash.h"

namespace cmudb {

template <typename T> class LRUReplacer : public Replacer<T> {
  struct Node {
    Node() = default;
    explicit Node(T d, Node* p = nullptr): data(d), prev(p){}
    T data;
    Node *prev = nullptr;
    std::unique_ptr<Node> next;
  };
public:
  // do not change public interface
  LRUReplacer();

  ~LRUReplacer();

  void Insert(const T &value);

  bool Victim(T &value);

  bool Erase(const T &value);

  size_t Size();

private:
  // add your member variables here
  mutable std::mutex mtx;
  std::unordered_map<T, Node*> hashmap;
  std::unique_ptr<Node> head;
  Node* tail;
};

} // namespace cmudb
