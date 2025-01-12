// Copyright: ThoughtSpot Inc. 2021
// Author: Mohit Saini (mohit.saini@thoughtspot.com)

#include "ts/lazy_map.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "gtest/gtest.h"
#include "glog/logging.h"

#include "testing/cpp/copy_move_counter.hpp"

using std::vector;
using std::cout;
using std::endl;
using testing::cpp::CopyMoveCounter;

namespace ts {

template<typename M>
std::unordered_set<typename M::key_type> GetKeys(const M& m) {
  std::unordered_set<typename M::key_type> output;
  for (auto& e : m) {
    output.insert(e.first);
  }
  return output;
}

TEST(LazyMapTest, Basic) {
  lazy_map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
  EXPECT_EQ(3, m.size());
  m.insert(4, 40);
  EXPECT_EQ(4, m.size());
  EXPECT_EQ(40, m.at(4));
  m.insert_or_assign(3, 50);
  EXPECT_EQ(50, m.at(3));
  EXPECT_EQ(4, m.size());
  EXPECT_TRUE(m.contains(1));
  m.erase(1);
  EXPECT_FALSE(m.contains(1));
  EXPECT_EQ(3, m.size());
  EXPECT_TRUE(m.contains(2));
  m.clear();
  EXPECT_FALSE(m.contains(2));
  EXPECT_EQ(0, m.size());
  m.insert({10, 20 + 30});
  EXPECT_EQ(1, m.size());
  EXPECT_NE(m.find(10), m.end());
  EXPECT_EQ(50, m.find(10)->second);
}

TEST(LazyMapTest, PreserveValueSemantics) {
  lazy_map<int, int> m1 = {{1, 10}, {2, 20}, {3, 30}};
  auto m2 = m1;
  EXPECT_EQ(3, m2.size());
  m2.insert(4, 40);
  EXPECT_EQ(4, m2.size());
  EXPECT_EQ(40, m2.at(4));
  EXPECT_EQ(3, m1.size());
  EXPECT_FALSE(m1.contains(4));
  m1.insert_or_assign(3, 50);
  EXPECT_EQ(50, m1.at(3));
  EXPECT_EQ(3, m1.size());
  EXPECT_EQ(30, m2.at(3));
  EXPECT_EQ(4, m2.size());
  auto m3 = m2;
  EXPECT_EQ(4, m3.size());
  EXPECT_EQ(10, m3.at(1));
  EXPECT_TRUE(m3.contains(4));
  EXPECT_FALSE(m3.contains(5));
  m3.erase(1);
  EXPECT_FALSE(m3.contains(1));
  EXPECT_TRUE(m1.contains(1));
  EXPECT_TRUE(m2.contains(1));
  EXPECT_EQ(3, m3.size());
  EXPECT_EQ(3, m1.size());
  EXPECT_EQ(4, m2.size());
  m3.clear();
  EXPECT_EQ(0, m3.size());
  EXPECT_EQ(3, m1.size());
  EXPECT_EQ(4, m2.size());
}


TEST(LazyMapTest, DetachmentTest) {
  lazy_map<int, int> m1 = {{1, 10}, {2, 20}, {3, 30}};
  auto m2 = m1;
  m2.insert(4, 40);
  auto m3 = m2;
  m3.insert(5, 50);
  m3.erase(3);
  EXPECT_EQ((std::unordered_set<int> {1, 2, 3, 4}), GetKeys(m2));
  EXPECT_EQ((std::unordered_set<int> {1, 2, 4, 5}), GetKeys(m3));
  EXPECT_TRUE(m2.detach());
  EXPECT_FALSE(m2.detach());
  EXPECT_TRUE(m2.is_detached());
  EXPECT_TRUE(m3.detach());
  auto m4 = m3;
  m4.insert(6, 60);
  EXPECT_TRUE(m4.detach());
}

TEST(LazyMapTest, IteratorTest) {
  lazy_map<int, int> m1 = {{1, 10}, {2, 20}, {3, 30}};
  auto m2 = m1;
  m2.insert(4, 40);
  m2.detach();
  std::set<std::pair<int, int>> v1(m2.begin(), m2.end());
  std::set<std::pair<int, int>> v2 = {{1, 10}, {2, 20}, {3, 30}, {4, 40}};
  EXPECT_EQ(v2, v1);
  EXPECT_FALSE(m2.detach());
  EXPECT_FALSE(m1.detach());
  auto m3 = m2;
  m2.insert(5, 50);
  std::unordered_set<int> v3;
  for (const auto& item : m2) {
    v3.insert(item.second - item.first);
  }
  EXPECT_EQ((std::unordered_set<int> {9, 18, 27, 36, 45}), v3);
  auto m4 = m3;
  m4.erase(3);
  m4.insert_or_assign(2, 21);
  EXPECT_EQ(std::unordered_set<int>({4, 1, 2}), GetKeys(m4));
  auto m5 = m4;
  m5.clear();
  EXPECT_EQ(3, GetKeys(m4).size());
  m5 = m4;
  m5.insert(12, 33);
  EXPECT_EQ(std::unordered_set<int>({4, 1, 2, 12}), GetKeys(m5));
  m5.erase(12);
  auto m6 = m5;
  EXPECT_EQ(2, m6.get_depth());
  m6.insert(13, 33);
  EXPECT_EQ(std::unordered_set<int>({4, 1, 2, 13}), GetKeys(m6));
  EXPECT_EQ(3, m6.get_depth());
  lazy_map<int, int> m7({{1, 10}});
  auto m8 = m7;
  m7.erase(1);
  EXPECT_EQ(std::unordered_set<int>({}), GetKeys(m7));
}

TEST(LazyMapTest, CopyMoveInsertion) {
  ts::lazy_map<int, CopyMoveCounter> m;
  CopyMoveCounter::Info info;
  m.insert(10, CopyMoveCounter(&info));
  EXPECT_EQ(1, info.moves());
  EXPECT_EQ(0, info.copies());
  EXPECT_EQ(1, info.value_ctr);
  info.reset();
  m.emplace(20, &info);
  EXPECT_EQ(0, info.moves());
  EXPECT_EQ(0, info.copies());
  EXPECT_EQ(1, info.value_ctr);
  CopyMoveCounter c1(&info);
  info.reset();
  m.insert_or_assign(10, std::move(c1));
  EXPECT_EQ(1, info.move_assign);
  EXPECT_EQ(1, info.total());
}

}  // namespace ts
