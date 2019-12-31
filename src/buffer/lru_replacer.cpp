/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace cmudb {

template <typename T> LRUReplacer<T>::LRUReplacer() {
  head = std::make_unique<Node>();
  tail = head.get();
}

template <typename T> LRUReplacer<T>::~LRUReplacer() {}

/*
 * Insert value into LRU
 */
template <typename T> void LRUReplacer<T>::Insert(const T &value) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = hashmap.find(value);
  if(it == hashmap.end()){
    tail->next = std::make_unique<Node>(value, tail);
    tail = tail->next.get();
    hashmap.emplace(value, tail);
  } else{
    if(it->second != tail){
      Node *previous = it->second->prev;
      std::unique_ptr<Node> current = std::move(previous->next);
      previous->next = std::move(current->next);
      previous->next->prev = previous;

      current->prev = tail;
      tail->next = std::move(current);
      tail = tail->next.get();
    }
  }
}

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
template <typename T> bool LRUReplacer<T>::Victim(T &value) {
  std::lock_guard<std::mutex> lock(mtx);

  if (hashmap.size() == 0){
    return false;
  }

  value = head->next->data;
  head->next = std::move(head->next->next);
  if(head->next != nullptr){
    head->next->prev = head.get();
  }
  hashmap.erase(value);
  if(!hashmap.size()) tail = head.get();

  return true;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = hashmap.find(value);
  if(it == hashmap.end()) return false;
  if(it->second != tail){
    Node* previous = it->second->prev;
    Node* current = std::move(it->second);
    previous->next = std::move(current->next);
    previous->next->prev = previous;
  }else{
    tail = tail->prev;
    tail->next.release();
  }
  hashmap.erase(value);
  if(!hashmap.size()){
    tail = head.get();
  }
  return true;
}

template <typename T> size_t LRUReplacer<T>::Size() { 
  std::lock_guard<std::mutex> lock(mtx);
  return hashmap.size(); 
}

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace cmudb
