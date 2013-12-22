#include "gtest/gtest.h"
#include "hash_table.hh"

using namespace yalp;

typedef const char* Key;
typedef const char* Value;

struct TestHashPolicy : public HashPolicy<Key> {
  virtual unsigned int hash(const Key a) override {
    return strlen(a);
  }
  virtual bool equal(const Key a, const Key b) override {
    return strcmp(a, b) == 0;
  }
};

class HashTableTest : public ::testing::Test {
protected:
  virtual void SetUp() override {
    allocator_ = Allocator::create(getDefaultAllocFunc(), NULL);
  }

  virtual void TearDown() override {
    allocator_->release();
  }

  TestHashPolicy policy_;
  Allocator* allocator_;
};

TEST_F(HashTableTest, PutGet) {
  HashTable<Key, Value> ht = HashTable<Key, Value>(&policy_, allocator_);

  ASSERT_TRUE(NULL == ht.get("foo")) << "get is failed for empty table";

  ht.put("foo", "hoge");
  ASSERT_TRUE(NULL != ht.get("foo")) << "get is succeeded for existing key";
  ASSERT_STREQ("hoge", *ht.get("foo"));

  // "bar" is same hash value for "foo", because hash function
  // is length of string.

  ASSERT_TRUE(NULL == ht.get("bar")) << "get is failed for existing hash key value";
  ht.put("bar", "fuga");
  ASSERT_STREQ("fuga", *ht.get("bar")) << "put preserves conflicting hash key value";

  // Key can be removed.
  ASSERT_TRUE(ht.remove("foo"));
  ASSERT_TRUE(NULL == ht.get("foo"));
  ASSERT_TRUE(ht.remove("bar"));
  ASSERT_TRUE(NULL == ht.get("bar"));
  ASSERT_FALSE(ht.remove("baz"));
}

TEST_F(HashTableTest, Each) {
  HashTable<Key, Value> ht = HashTable<Key, Value>(&policy_, allocator_);

  ht.put("1", "one");
  ht.put("22", "two");
  ht.put("333", "three");

  HashTable<Key, Value>::const_iterator it = ht.begin();
  ASSERT_NE(ht.end(), it);
  ASSERT_STREQ("1", it->key);
  ASSERT_STREQ("one", it->value);

  ++it;
  ASSERT_NE(ht.end(), it);
  ASSERT_STREQ("22", it->key);
  ASSERT_STREQ("two", it->value);

  ++it;
  ASSERT_NE(ht.end(), it);
  ASSERT_STREQ("333", it->key);
  ASSERT_STREQ("three", it->value);

  ++it;
  ASSERT_EQ(ht.end(), it);
}
