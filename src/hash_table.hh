//=============================================================================
/// Hash table
/*
 * Map Key to Value in O(1) using hash function.
 *
 * Variable declaration:
 *   HashTable<K, V, P> ht;
 *
 * Put element:
 *   ht.put(k, v);
 *
 * Get element:
 *   V* pv = ht.get(k);
 *
 * Remove element for key:
 *   ht.remove(k);
 *
 * Each:
 *   for (HashTable<K, V, P>::const_iterator it = ht.begin();
 *        it != ht.end(); ++it) {
 *     // Key access: it->key
 *     // Value access: it->value
 *   }
 *     or using range-based for
 *   for (auto kv : ht) {
 *     // Key access: kv.key
 *     // Value access: kv.value
 *   }
 *
 *
 * Policy:
 *   Policy class determines the behavior of hash table.
 *   it must implement following 2 static functions:
 *
 *   struct Policy {
 *     static unsigned int hash(const Key a) {
 *       // return some unsigned integer to distribute keys.
 *     }
 *     static bool equal(const Key a, const Key b) {
 *       // return true if 2 keys are same.
 *     }
 *   };
 */
//=============================================================================

#ifndef _HASH_TABLE_HH_
#define _HASH_TABLE_HH_

#include "allocator.hh"

namespace yalp {

#ifndef NULL
#define NULL  (0)
#endif

template <class Key>
struct HashPolicy {
  // return some unsigned integer to distribute keys.
  virtual unsigned int hash(Key a) = 0;
  // return true if 2 keys are same.
  virtual bool equal(Key a, Key b) = 0;
};

// Hash table
template <class Key, class Value>
class HashTable {
public:
  static const unsigned int INITIAL_BUFFER_SIZE = 5;

  explicit HashTable(HashPolicy<Key>* policy, Allocator* allocator)
    : policy_(policy), allocator_(allocator)
    , array_(NULL), arraySize_(0)
    , entryCount_(0), conflictCount_(0) {
  }

  explicit HashTable(HashPolicy<Key>* policy, unsigned int capacity)
    : policy_(policy), array_(NULL), arraySize_(capacity)
    , entryCount_(0), conflictCount_(0) {
  }

  ~HashTable() {
    if (array_ != NULL) {
      for (unsigned int i = 0; i < arraySize_; ++i) {
        for (Link* link = array_[i]; link != NULL; ) {
          Link* next = link->next;
          allocator_->free(link);
          link = next;
        }
      }
      allocator_->free(array_);
    }
  }

  unsigned int getCapacity() const  { return arraySize_; }
  int getEntryCount() const  { return entryCount_; }
  int getConflictCount() const  { return conflictCount_; }
  int getMaxDepth() const  { return calcMaxDepth(); }

  void put(const Key key, const Value& value) {
    Link* link = find(key);
    if (link == NULL)
      link = createLink(key);
    link->value = value;
  }

  const Value* get(const Key key) const {
    Link* link = find(key);
    if (link == NULL)
      return NULL;
    return &link->value;
  }

  bool remove(const Key key) {
    Link* prev;
    unsigned int index;
    Link* link = find(key, &prev, &index);
    if (link == NULL)
      return false;
    if (prev == NULL)
      array_[index] = link->next;
    else
      prev->next = link->next;
    allocator_->free(link);
    return true;
  }

private:
  struct Link {
    Link* next;
    Key key;
    Value value;
  };

public:
  // const_iterator
  class const_iterator {
  public:
    const const_iterator& operator++() {
      link = link->next;
      if (link == NULL) {
        do {
          if (++index >= ht->arraySize_)
            break;
          link = ht->array_[index];
        } while (link == NULL);
      }
      return *this;
    }

    bool operator==(const const_iterator& it) const  { return link == it.link; }
    bool operator!=(const const_iterator& it) const  { return link != it.link; }

    const Link* operator->() const  { return link; }
    const Link& operator*() const  { return *link; }

  private:
    const_iterator(const HashTable* ht, unsigned int index, Link* link) {
      this->ht = ht;
      this->index = index;
      this->link = link;
    }

    const HashTable* ht;
    unsigned int index;
    Link* link;
    friend class HashTable;
  };

  const_iterator begin() const {
    Link* link = NULL;
    unsigned int index = 0;
    for (index = 0; index < arraySize_; ++index) {
      link = array_[index];
      if (link != NULL)
        break;
    }
    const_iterator it(this, index, link);
    return it;
  }
  const_iterator end() const {
    return const_iterator(this, arraySize_, NULL);
  }

private:
  Link* find(const Key key, Link** pPrev = NULL, unsigned int* pIndex = NULL) const {
    if (array_ == NULL)
      return NULL;
    unsigned int hash = policy_->hash(key);
    unsigned int index = hash % arraySize_;
    Link* prev = NULL;
    for (Link* link = array_[index]; link != NULL;
         prev = link, link = link->next) {
      if (policy_->equal(key, link->key)) {
        if (pPrev != NULL)
          *pPrev = prev;
        if (pIndex != NULL)
          *pIndex = index;
        return link;
      }
    }
    return NULL;
  }

  Link* createLink(const Key key) {
    if (array_ == NULL || entryCount_ >= arraySize_)
      expand();
    unsigned int hash = policy_->hash(key);
    Link* link = new(allocator_->alloc(sizeof(*link))) Link;
    unsigned int index = hash % arraySize_;
    link->next = array_[index];
    link->key = key;
    if (array_[index] != NULL)
      ++conflictCount_;
    array_[index] = link;
    ++entryCount_;
    return link;
  }

  void expand() {
    unsigned int newSize = arraySize_;
    if (newSize < INITIAL_BUFFER_SIZE)
      newSize = INITIAL_BUFFER_SIZE;
    else if (newSize < 128)
      newSize <<= 1;
    else
      newSize = newSize + (newSize >> 1);  // x 1.5
    newSize += 1 - (newSize & 1);  // Force odd number.

    Link** newArray = static_cast<Link**>(allocator_->alloc(sizeof(Link*) * newSize));
    for (unsigned int i = 0; i < newSize; ++i)
      newArray[i] = NULL;
    rehash(array_, arraySize_, newArray, newSize, policy_);

    if (array_ != NULL)
      allocator_->free(array_);
    array_ = newArray;
    arraySize_ = newSize;
    conflictCount_ = 0;
    // Keeps entryCount_
  }

  static void rehash(Link** oldArray, unsigned int oldSize,
                     Link** newArray, unsigned int newSize, HashPolicy<Key>* policy) {
    if (oldArray == NULL || oldSize == 0)
      return;
    for (unsigned int i = 0; i < oldSize; ++i) {
      for (Link* link = oldArray[i]; link != NULL; ) {
        Link* next = link->next;
        unsigned int hash = policy->hash(link->key);
        unsigned int index = hash % newSize;
        link->next = newArray[index];
        newArray[index] = link;
        link = next;
      }
    }
  }

  int calcMaxDepth() const {
    int max = 0;
    for (unsigned int i = 0; i < arraySize_; ++i) {
      int depth = 0;
      for (Link* link = array_[i]; link != NULL; link = link->next)
        ++depth;
      if (depth > max)
        max = depth;
    }
    return max;
  }

  HashPolicy<Key>* policy_;
  Allocator* allocator_;
  Link** array_;
  unsigned int arraySize_;
  unsigned int entryCount_;  // Number of entries.
  unsigned int conflictCount_;  // Number of hash index conflicts.
};

}  // namespace yalp

#endif
