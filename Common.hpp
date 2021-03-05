#ifndef INCLUDE_GUARD_Common
#define INCLUDE_GUARD_Common


#include "/scripts/code/dcheck.hpp"

#include <tuple>
#include <stdint.h> // include uint64_t etc.
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <stack>

template<class T>
class PersistentInitialStack {
  public:
  typedef T value_type;
  typedef typename std::stack<T>::size_type size_type;


  private:
  std::vector<T> m_persistent_stack;
  size_type m_persistent_size = 0; // simulates m_persistent_stack.size()
  std::vector<T> m_mod_stack;
  bool m_initializing = true; // switch the states at the first pop()
  size_t m_persistent_depth = 0;

  public:

  // PersistentInitialStack& operator=(PersistentInitialStack&& o) {
  //   this->m_persistent_stack = std::move(o.m_persistent_stack);
  //   this->m_mod_stack = std::move(o.m_mod_stack);
  //   this->m_initializing = o.m_initializing;
  //   this->m_persistent_depth = o.m_persistent_depth;
  //   this->m_persistent_size = o.m_persistent_size;
  //   std::cout << "ok";
  //   return *this;
  // }
  bool empty() const {
    return size() == 0;
  }

  void reset() {
    m_initializing = false;
    m_mod_stack.clear();
    m_persistent_size = m_persistent_stack.size();
  }
  size_type size() const {
    if(m_initializing) {
      return m_persistent_stack.size();
    }
    return m_persistent_size + m_mod_stack.size();
  }

  value_type& top() {
    if(!m_mod_stack.empty()) {
      return m_mod_stack.back();
    }

    DCHECK_LE(m_persistent_size, m_persistent_stack.size());
    return m_persistent_stack[m_persistent_size-1];
  }
  const value_type& top() const {
    return m_persistent_stack.back();
  }
  void pop() {
    if(m_initializing) {
      m_initializing = false;
      DCHECK_EQ(m_persistent_size, m_persistent_stack.size());
      DCHECK(m_mod_stack.empty());
    }
    if(m_mod_stack.empty()) {
      DCHECK_GT(m_persistent_size, 0);
      --m_persistent_size;
      return;
    } 
    m_mod_stack.pop_back();
  }
  void push(const T& value ) {
    if(m_initializing) {
      m_persistent_stack.push_back(value);
      ++m_persistent_size;
      DCHECK_EQ(m_persistent_size, m_persistent_stack.size());
      return;
    } 
    m_mod_stack.push_back(value);
  }
};

template<typename var_t>
struct PairT
{
  var_t left, right;

  bool operator<(const PairT & another) const
  {
    return (this->left < another.left) || ((this->left == another.left) && this->right < another.right);
  };
};


void padVLine
(
 const uint64_t pad
 ) {
  for (uint64_t i = 0; i < pad; ++i) {
    std::cout << "|";
  }
}


uint32_t ceilLog2(uint64_t x) {
  if (x == 0) {
    return 1;
  }
  return 64 - __builtin_clzll(x);
}


template<class type>
void printArray(type arr, uint64_t n, std::string delimiter = " ")
{
  for (uint64_t i = 0; i < n; ++i) {
    std::cout << arr[i] << delimiter;
  }
  std::cout << std::endl;
}


template<class Container>
void printVec(const Container & vec)
{
  for (uint64_t i = 0; i < vec.size(); ++i) {
    std::cout << "(" << i << ":" << vec[i] << ") ";
  }
  std::cout << std::endl;
}


template<class SlpT>
void decompressByCharAt
(
 const SlpT & slp,
 const std::string & ofile
) {
  std::ofstream ofs(ofile);
  for (uint64_t i = 0; i < slp.getLen(); ++i) {
    char c = slp.charAt(i);
    ofs.write(&c, 1);
  }
}


template<class SlpT>
void decompressByRecurse
(
 const SlpT & slp,
 const std::string & ofile
) {
  using var_t = typename SlpT::var_t;
  const uint64_t alphSize = slp.getAlphSize();
  std::ofstream ofs(ofile);
  for (uint64_t i = 0; i < slp.getLenSeq(); ++i) {
    const var_t var = slp.getSeq(i);
    std::stack<std::pair<var_t, uint64_t> > st;
    st.push(std::make_pair(var, 0));
    while (!st.empty()) {
      auto & e = st.top();
      if (e.first < alphSize) { // leaf
        char c = slp.getChar(e.first);
        ofs.write(&c, 1);
        st.pop();
      } else if (e.second == 0) {
        e.second = 1;
        st.push(std::make_pair(slp.getLeft(e.first - alphSize), 0));
      } else if (e.second == 1) {
        e.second = 2;
        st.push(std::make_pair(slp.getRight(e.first - alphSize), 0));
      } else {
        st.pop();
      }
    }
  }
}


template<class SlpT>
void printDerivationTree
(
 const SlpT & slp
) {
  std::stack<typename SlpT::nodeT> path;
  path.push(slp.getRootNode());
  if (path.size() == 0) {
    return;
  }
  while (true) {
    auto node = path.top();
    padVLine(path.size() - 1);
    std::cout << " (" << std::get<0>(node) << ", " << std::get<1>(node) << ", " << std::get<2>(node) << ")";
    if (std::get<0>(node) == 1) {
      std::cout << " " << (char)(std::get<1>(node));
    }
    std::cout << std::endl;
    if (std::get<0>(node) == 1) {
      if (!proceedPrefixPath(slp, path)) {
        return;
      }
    } else {
      descentPrefixPath(slp, path, std::get<0>(node) - 1);
    }
  }
}



template<class SlpT, class stackT = std::stack<typename SlpT::nodeT> >
void getPrefixPath
(
 const SlpT & slp,
 stackT & path,
 uint64_t pos
) {
  if (pos >= slp.getLen()) {
    return;
  }
  path.push(slp.getRootNode());
  if (pos) {
    path.push(slp.getChildNodeForPos_Root(pos));
  }
  while (pos) {
    path.push(slp.getChildNodeForPos(path.top(), pos)); // pos is modified to relative pos in a node
  }
}


/*!
 * modify the stack 'path' to point the highest node that is adjacent to the node path.top()
 * return false when such a node does not exist
 */
template<class SlpT, class stackT = PersistentInitialStack<typename SlpT::nodeT> >
bool proceedPrefixPath
(
 const SlpT & slp,
 stackT & path
 ) {
  if (path.size() <= 1) {
    return false;
  }
  typename SlpT::nodeT n;
  do {
    n = path.top();
    path.pop();
  } while (path.size() > 1 and std::get<2>(n) == 1);
  if (path.size() > 1) {
    path.push(slp.getChildNode(path.top(), 1));
  } else { // add (std::get<2>(n) + 1)th (0base) child of root
    if (std::get<2>(n) + 1 < slp.getLenSeq()) {
      path.push(slp.getChildNode_Root(std::get<2>(n) + 1));
    } else {
      return false;
    }
  }
  return true;
}


template<class SlpT, class stackT = std::stack<typename SlpT::nodeT> >
void descentPrefixPath
(
 const SlpT & slp,
 stackT & path,
 const uint64_t len
 ) {
  auto n = (path.size() == 1) ? slp.getChildNode_Root(0) : slp.getChildNode(path.top(), 0);
  path.push(n);
  while (std::get<0>(n) > len) {
    n = slp.getChildNode(path.top(), 0);
    path.push(n);
  }
}


template<class SlpT>
uint64_t lceToR
(
 const SlpT & slp,
 const uint64_t p1,
 const uint64_t p2
) {
  std::stack<typename SlpT::nodeT> path1, path2;

  getPrefixPath(slp, path1, p1);
  getPrefixPath(slp, path2, p2);

  uint64_t l = 0;
  while (true) {
    auto n1 = path1.top();
    auto n2 = path2.top();
    while (std::get<0>(n1) != std::get<0>(n2)) {
      if (std::get<0>(n1) > std::get<0>(n2)) {
        descentPrefixPath(slp, path1, std::get<0>(n2));
        n1 = path1.top();
        // std::cout << "descent n1: " << std::get<0>(n1) << ", " << std::get<1>(n1) << ", " << std::get<2>(n1) << std::endl;
      } else {
        descentPrefixPath(slp, path2, std::get<0>(n1));
        n2 = path2.top();
        // std::cout << "descent n2: " << std::get<0>(n2) << ", " << std::get<1>(n2) << ", " << std::get<2>(n2) << std::endl;
      }
    }
    if (std::get<1>(n1) == std::get<1>(n2)) { // match
      l += std::get<0>(n1);
      if (!(proceedPrefixPath(slp, path1))) {
        break;
      }
      if (!(proceedPrefixPath(slp, path2))) {
        break;
      }
    } else if (std::get<0>(n1) > 1) { // mismatch with non-terminal
      descentPrefixPath(slp, path1, std::get<0>(n1) - 1);
      descentPrefixPath(slp, path2, std::get<0>(n1) - 1);
    } else { // lce ends with mismatch char
      break;
    }
  }
  return l;
}



template<class SlpT, class stackT = PersistentInitialStack<typename SlpT::nodeT> >
uint64_t lceToRBounded
(
 const SlpT & slp,
 const uint64_t p1,
 const uint64_t p2,
 const uint64_t upperbound,
 stackT& path1
) {
  stackT path2;

  if(path1.empty()) {
    getPrefixPath(slp, path1, p1);
  }
  getPrefixPath(slp, path2, p2);

  uint64_t l = 0;
  while (true) {
    auto n1 = path1.top();
    auto n2 = path2.top();
    while (std::get<0>(n1) != std::get<0>(n2)) {
      if (std::get<0>(n1) > std::get<0>(n2)) {
        descentPrefixPath(slp, path1, std::get<0>(n2));
        n1 = path1.top();
        // std::cout << "descent n1: " << std::get<0>(n1) << ", " << std::get<1>(n1) << ", " << std::get<2>(n1) << std::endl;
      } else {
        descentPrefixPath(slp, path2, std::get<0>(n1));
        n2 = path2.top();
        // std::cout << "descent n2: " << std::get<0>(n2) << ", " << std::get<1>(n2) << ", " << std::get<2>(n2) << std::endl;
      }
    }
    if (std::get<1>(n1) == std::get<1>(n2)) { // match
      l += std::get<0>(n1);
      if(l >= upperbound) { return l; }
      if (!(proceedPrefixPath(slp, path1))) {
        break;
      }
      if (!(proceedPrefixPath(slp, path2))) {
        break;
      }
    } else if (std::get<0>(n1) > 1) { // mismatch with non-terminal
      descentPrefixPath(slp, path1, std::get<0>(n1) - 1);
      descentPrefixPath(slp, path2, std::get<0>(n1) - 1);
    } else { // lce ends with mismatch char
      break;
    }
  }
  return l;
}


template<class SlpT>
uint64_t lceToR_Naive
(
 const SlpT & slp,
 const uint64_t p1,
 const uint64_t p2
) {
  std::stack<typename SlpT::nodeT> path1, path2;

  getPrefixPath(slp, path1, p1);
  getPrefixPath(slp, path2, p2);

  uint64_t l = 0;
  while (true) {
    auto n1 = path1.top();
    auto n2 = path2.top();
    if (std::get<0>(n1) != 1) {
      descentPrefixPath(slp, path1, 1);
      n1 = path1.top();
    }
    if (std::get<0>(n2) != 1) {
      descentPrefixPath(slp, path2, 1);
      n2 = path2.top();
    }
    if (std::get<1>(n1) == std::get<1>(n2)) { // match char
      ++l;
      if (!(proceedPrefixPath(slp, path1))) {
        break;
      }
      if (!(proceedPrefixPath(slp, path2))) {
        break;
      }
    } else { // lce ends with mismatch char
      break;
    }
  }
  return l;
}

template<class SlpT>
uint64_t lceToR_NaiveBounded
(
 const SlpT & slp,
 const uint64_t p1,
 const uint64_t p2,
 const uint64_t upperbound
) {
  std::stack<typename SlpT::nodeT> path1, path2;

  getPrefixPath(slp, path1, p1);
  getPrefixPath(slp, path2, p2);

  uint64_t l = 0;
  while (true) {
    auto n1 = path1.top();
    auto n2 = path2.top();
    if (std::get<0>(n1) != 1) {
      descentPrefixPath(slp, path1, 1);
      n1 = path1.top();
    }
    if (std::get<0>(n2) != 1) {
      descentPrefixPath(slp, path2, 1);
      n2 = path2.top();
    }
    if (std::get<1>(n1) == std::get<1>(n2)) { // match char
      ++l;
      if(l >= upperbound) { return l; }
      if (!(proceedPrefixPath(slp, path1))) {
        break;
      }
      if (!(proceedPrefixPath(slp, path2))) {
        break;
      }
    } else { // lce ends with mismatch char
      break;
    }
  }
  return l;
}


// template<class SlpT>
// uint64_t lceToR
// (
//  const SlpT & slp,
//  const uint64_t p1,
//  const uint64_t p2
// ) {
//   std::stack<typename SlpT::nodeT> s1, s2;
//   slp.init_LceToR(s1, p1);
//   slp.init_LceToR(s2, p2);

//   // {
//   //   std::stack<typename SlpT::nodeT> s;
//   //   slp.init_LceToR(s, p1);
//   //   std::cout << "p1 = " << p1 << std::endl;
//   //   while (!s.empty()) {
//   //     std::cout << std::get<0>(s.top()) << ", " << std::get<1>(s.top()) << ", " << std::get<2>(s.top()) << std::endl;
//   //     s.pop();
//   //   }
//   // }
//   // {
//   //   std::stack<typename SlpT::nodeT> s;
//   //   slp.init_LceToR(s, p2);
//   //   std::cout << "p2 = " << p2 << std::endl;
//   //   while (!s.empty()) {
//   //     std::cout << std::get<0>(s.top()) << ", " << std::get<1>(s.top()) << ", " << std::get<2>(s.top()) << std::endl;
//   //     s.pop();
//   //   }
//   // }

//   uint64_t l = 0;
//   while (true) {
//     auto n1 = s1.top();
//     auto n2 = s2.top();
//     while (std::get<0>(n1) != std::get<0>(n2)) {
//       if (std::get<0>(n1) > std::get<0>(n2)) {
//         slp.descent_LceToR(s1, std::get<0>(n2));
//         n1 = s1.top();
//         // std::cout << "descent n1: " << std::get<0>(n1) << ", " << std::get<1>(n1) << ", " << std::get<2>(n1) << std::endl;
//       } else {
//         slp.descent_LceToR(s2, std::get<0>(n1));
//         n2 = s2.top();
//         // std::cout << "descent n2: " << std::get<0>(n2) << ", " << std::get<1>(n2) << ", " << std::get<2>(n2) << std::endl;
//       }
//     }
//     if (std::get<1>(n1) == std::get<1>(n2)) { // match
//       l += std::get<0>(n1);
//       if (!(slp.next_LceToR(s1))) {
//         break;
//       }
//       if (!(slp.next_LceToR(s2))) {
//         break;
//       }
//     } else if (std::get<0>(n1) > 1) {
//       slp.descent_LceToR(s1, std::get<0>(n1) - 1);
//       slp.descent_LceToR(s2, std::get<0>(n1) - 1);
//     } else { // lce ends with mismatch char
//       break;
//     }
//   }
//   return l;
// }


template<typename ArrayT>
class PackedArrayTypeValRef
{
  friend ArrayT;


private:
  ArrayT * const obj_;
  const uint64_t idx_;


  PackedArrayTypeValRef
  (
   ArrayT * obj,
   uint64_t idx
   ) :
    obj_(obj),
    idx_(idx)
  {}


public:
  uint64_t operator=
  (
   uint64_t val
   ) {
    obj_->write(val, idx_);
    return val;
  }


  operator uint64_t() const {
    return obj_->read(idx_);
  }
};




/*!
 * @tparam kBucketWidth: Bitwidth of bucket size
 * @tparam elem_t: type of element to be sorted
 * @tparam
 *   Func: Fuction that returns kBucketWidth width integer from an element
 */
template<uint8_t kBucketWidth = 8, class elem_t, class Func>
void my_bucket_sort
(
 elem_t * earray, //!< given array to be sorted by some criterion specified by func
 uint64_t n, //!< length of array
 Func func
 ) {
  const uint64_t kBS = UINT64_C(1) << kBucketWidth; // bucket size
  std::queue<elem_t> bucket[kBS];
  for (uint64_t i = 0; i < n; ++i) {
    auto e = earray[i];
    bucket[func(e)].push(e);
  }
  uint64_t i = 0;
  for (uint64_t k = 0; k < kBS; ++k) {
    while (!bucket[k].empty()) {
      elem_t e = bucket[k].front();
      bucket[k].pop();
      earray[i++] = e;
    }
  }
}


/*!
 * @tparam kBucketWidth: Bitwidth of bucket size
 * @tparam elem_t: type of element to be sorted
 * @tparam
 *   Func: Fuction that returns kBucketWidth width integer from an element
 */
template<uint8_t kBucketWidth = 8, class elem_t, class keys_t>
void my_radix_sort
(
 elem_t * earray, //!< given array to be sorted by some criterion specified by func
 keys_t * keys, //!< i \in [0..n) is sorted based on keys[i]
 uint64_t n, //!< length of array
 uint8_t keyWidth
 ) {
  const uint64_t mask = (UINT64_C(1) << kBucketWidth) - 1; // bucket size
  for (uint64_t k = 0; k < (keyWidth + kBucketWidth - 1) / kBucketWidth; ++k) {
    my_bucket_sort<8>
      (earray, n, 
       [keys, k](uint64_t i){
         return (keys[i] >> (kBucketWidth * k)) & mask;
       }
       );
  }
}



#endif
