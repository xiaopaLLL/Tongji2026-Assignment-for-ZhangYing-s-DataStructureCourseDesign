// ============================================================================
//  题目六  家谱管理系统
//  文件结构：常量 → 单链表 List<T> → 一般树 TreeNode<T> / Tree<T>
//            → 哈希表 HashMap<K, V> → 控制台输入 Input → 家族成员 Member
//            → 家谱 FamilyTree
//  数据结构：用自己实现的一般树Tree<Member*>保存血缘关系（结点用数据域 + 儿女链表表
//           示，另用哈希表按姓名定位结点（平均O(1)）
//  复杂度：建家谱 O(1)；完善家谱（一次录入 k 个）O(k)；添加成员平均
//  O(1)；按姓名查找
//         平均 O(1)；改名平均 O(1)；解散局部家庭 O(m)（m 为被解散子树的人数）
//  说明：类与函数都定义在本文件里，T6.cpp里只有主函数；链表、树、哈希表都是自己写的，
//        没有使用任何 STL 容器
// ============================================================================

#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>

// ============================== 常量 ==============================
const unsigned int kNameLen = 20;             // 姓名最多 20 字节
const unsigned int kMaxChildCount = 100;      // 一个成员最多 100 个儿女
const unsigned int kMaxMemberCount = 100000;  // 家谱最多 10 万个成员
const unsigned int kNameLineLen = 500;  // 一次录入多个姓名时，一行最长 500 字节
const unsigned int kMenuWidth = 51;     // 菜单框宽度（与题目示例一致）

// ============================== 单链表 ==============================
template <typename T>
struct ListNode {
  T data;
  ListNode<T>* next;
  explicit ListNode(const T& value) : data(value), next(nullptr) {}
  explicit ListNode(T&& value) : data(static_cast<T&&>(value)), next(nullptr) {}
};

// 带头结点的单链表：头结点（哨兵）不存数据，尾指针让尾插是 O(1)。
// 它只管"链表结点"自己的生死；链表里存的对象由调用者负责释放。
template <typename T>
class List {
 public:
  List() {
    head_ = new ListNode<T>(T());
    tail_ = head_;
    size_ = 0;
  }

  // 深拷贝：链表结点是 new 出来的，两条链不能共用结点，否则析构时会重复释放
  List(const List& other) {
    head_ = new ListNode<T>(T());
    tail_ = head_;
    size_ = 0;
    copyFrom(other);
  }

  List& operator=(const List& other) {
    if (this != &other) {
      clear();
      copyFrom(other);
    }
    return *this;
  }

  ~List() {
    clear();
    delete head_;
  }

  void clear() {
    ListNode<T>* p = head_->next;
    while (p != nullptr) {
      ListNode<T>* next = p->next;
      delete p;
      p = next;
    }
    head_->next = nullptr;
    tail_ = head_;
    size_ = 0;
  }

  void push_back(const T& value) {
    ListNode<T>* node = new ListNode<T>(value);
    tail_->next = node;
    tail_ = node;
    size_++;
  }

  // 只移动不拷贝的元素（比如持有子树的 ChildRef）用这个重载
  void push_back(T&& value) {
    ListNode<T>* node = new ListNode<T>(static_cast<T&&>(value));
    tail_->next = node;
    tail_ = node;
    size_++;
  }

  // 删除第一个满足条件的元素（已经定位到，摘链是 O(1)）
  template <typename Func>
  bool eraseFirst(Func pred) {
    ListNode<T>* p = head_;
    while (p->next != nullptr) {
      if (pred(p->next->data)) {
        ListNode<T>* target = p->next;
        p->next = target->next;
        if (target == tail_) tail_ = p;
        delete target;
        size_--;
        return true;
      }
      p = p->next;
    }
    return false;
  }

  // 从头到尾访问每个元素（只读），回调拿到的是一份 const 引用
  template <typename Func>
  void forEach(Func func) const {
    for (const ListNode<T>* p = head_->next; p != nullptr; p = p->next)
      func(p->data);
  }

  unsigned int size() const { return size_; }

  // 第一个数据结点（配合 ListNode<T>::next 可以顺着往下走完整条链）
  ListNode<T>* first() const { return head_->next; }

 private:
  void copyFrom(const List& other) {
    for (const ListNode<T>* p = other.head_->next; p != nullptr; p = p->next)
      push_back(p->data);
  }

  ListNode<T>* head_;
  ListNode<T>* tail_;
  unsigned int size_;
};

// ============================== 一般树 ==============================
template <typename T>
class TreeNode;
template <typename T>
class Tree;

// 链表里存的"儿女"：一个持有子结点的包装，析构时就 delete 掉它。
// 这样一来，List 本来就要做的"逐个删链表结点"正好顺手把子树释放掉。
// 所有权唯一，所以不可拷贝、也不做移动赋值（没这个需求，留着反而容易误删结点）。
template <typename T>
struct ChildRef {
  TreeNode<T>* node;

  ChildRef() : node(nullptr) {}
  explicit ChildRef(TreeNode<T>* n) : node(n) {}
  ChildRef(ChildRef&& other) : node(other.node) { other.node = nullptr; }
  ~ChildRef() {
    if (node != nullptr) delete node;  // 子结点析构会带走它的整棵子树
  }
  bool operator==(const ChildRef& other) const { return node == other.node; }

  ChildRef(const ChildRef&) = delete;
  ChildRef& operator=(const ChildRef&) = delete;
  ChildRef& operator=(ChildRef&&) = delete;
};

// 每个 TreeNode 拥有自己的整棵子树：儿女存在带头结点的单链表里，
// 结点析构时清空链表，链表析构逐个删掉 ChildRef，ChildRef 析构 delete 子结点，
// 子结点析构又清空它自己的链表——于是整棵子树逐层释放干净。
template <typename T>
class TreeNode {
 public:
  TreeNode() : value_(T()) {}
  explicit TreeNode(const T& value) : value_(value) {}

  TreeNode(const TreeNode&) = delete;  // 结点归它所在的树所有，禁止拷贝
  TreeNode& operator=(const TreeNode&) = delete;

  const T& value() const { return value_; }
  T& value() { return value_; }

  // 儿女链表：元素是持有子结点的 ChildRef，所以 size() 就是儿女数
  List<ChildRef<T> >& children() { return children_; }
  const List<ChildRef<T> >& children() const { return children_; }

  // 清空儿女链表：链表析构逐个删掉 ChildRef，ChildRef 析构再 delete 子结点，
  // 子结点析构又清空它自己的链表——整棵子树就这样逐层释放干净。
  ~TreeNode() { children_.clear(); }

 private:
  T value_;
  List<ChildRef<T> > children_;

  friend class Tree<T>;
};

// 一般树：树根是祖先，每个结点可以有任意多个儿女。
// 内部保留一个哨兵根结点：空树时 size_ 为 0，setRoot 只是给哨兵根赋值。
// 所有权：Tree 只负责"结点"的建立与释放；结点里装的 T 归调用者
// （家谱里装的是 Member*，由 FamilyTree 负责释放）。
template <typename T>
class Tree {
 public:
  Tree() : root_(), size_(0) {}

  Tree(const Tree&) = delete;
  Tree& operator=(const Tree&) = delete;
  ~Tree() { clear(); }

  bool empty() const { return size_ == 0; }
  unsigned int size() const { return size_; }

  TreeNode<T>* root() { return &root_; }

  // 建立树根：把哨兵根的值写上，结点数置 1
  void setRoot(const T& value) {
    root_.value() = value;
    size_ = 1;
  }

  // 改某个结点的值（改名、改其它字段都用它，不直接暴露结点内部）
  void setNode(TreeNode<T>* node, const T& value) {
    if (node != nullptr) node->value_ = value;
  }

  // 清空整棵树（只保留哨兵根）
  void clear() {
    root_.children().clear();
    root_.value() = T();
    size_ = 0;
  }

  // 给 parent 添加一个孩子（尾插到儿女链表，O(1)），返回新结点
  TreeNode<T>* addChild(TreeNode<T>& parent, const T& value) {
    TreeNode<T>* child = new TreeNode<T>(value);
    parent.children_.push_back(ChildRef<T>(child));
    size_++;
    return child;
  }

  // 前序遍历，用 same(结点值, value) 判等，返回第一个匹配的结点
  template <typename Func>
  TreeNode<T>* findValue(const T& value, Func same) {
    return findValueIn(&root_, value, same);
  }

  // 前序遍历整棵树每个结点
  template <typename Func>
  void forEachPreOrder(Func func) {
    if (empty()) return;
    forEachInSubtree(&root_,
                     [&func](TreeNode<T>* node) { func(node->value_); });
  }

  // 前序遍历以 node 为根的这棵子树里的每个结点（含 node 自己），回调拿到结点。
  // 要"边走边改"（比如解散局部家庭时逐个清索引、再释放成员）时用它。
  template <typename Func>
  void forEachInSubtree(TreeNode<T>* node, Func func) {
    if (node == nullptr || (node == &root_ && empty())) return;
    forEachIn(node, func);
  }

  // 只访问 node 的直接儿女（第一代子孙）
  template <typename Func>
  void forEachChild(TreeNode<T>* node, Func func) const {
    node->children_.forEach(
        [&func](const ChildRef<T>& ref) { func(ref.node); });
  }

  // 摘掉 node 并释放它整棵子树；node 是树根或不在树里时返回 false
  bool eraseSubtree(TreeNode<T>* node) {
    if (node == nullptr || node == &root_) return false;
    TreeNode<T>* parent = findParent(node);
    return parent != nullptr && eraseNode(parent, node);
  }

  // 已经知道父结点时用这个：省掉找父亲那一遍，删除只花 O(子树结点数)。
  // 注意：eraseFirst 删掉的是链表结点，而链表结点里的 ChildRef 析构会 delete
  // 它持有的子结点，于是整棵子树在这里就被释放了——不能再 delete 一次。
  bool eraseNode(TreeNode<T>* parent, TreeNode<T>* node) {
    if (parent == nullptr || node == nullptr || node == &root_) return false;
    unsigned int subtreeSize = 0;
    countNodes(node, subtreeSize);
    if (!parent->children_.eraseFirst(
            [node](const ChildRef<T>& ref) { return ref.node == node; }))
      return false;  // node 不是 parent 的孩子，什么都不动
    size_ -= subtreeSize;
    return true;
  }

  // 找到并删除第一棵值匹配的子树
  template <typename Func>
  bool removeValue(const T& value, Func same) {
    TreeNode<T>* node = findValue(value, same);
    return node != nullptr && eraseSubtree(node);
  }

 private:
  template <typename Func>
  static void forEachIn(TreeNode<T>* node, Func& func) {
    func(node);
    node->children_.forEach(
        [&func](const ChildRef<T>& ref) { forEachIn(ref.node, func); });
  }

  template <typename Func>
  static TreeNode<T>* findValueIn(TreeNode<T>* node, const T& value,
                                  Func same) {
    if (same(node->value_, value)) return node;
    TreeNode<T>* found = nullptr;
    node->children_.forEach([&found, &value, &same](const ChildRef<T>& ref) {
      if (found == nullptr) found = findValueIn(ref.node, value, same);
    });
    return found;
  }

  // 数子树里的结点个数（删除时用来扣 size_）
  static void countNodes(TreeNode<T>* node, unsigned int& count) {
    count++;
    node->children_.forEach(
        [&count](const ChildRef<T>& ref) { countNodes(ref.node, count); });
  }

  // 找 node 的父亲；node 就是树根或一次都没找到时返回 nullptr
  TreeNode<T>* findParent(TreeNode<T>* target) {
    if (target == &root_) return nullptr;
    TreeNode<T>* found = nullptr;
    root_.children_.forEach([&found, this, target](const ChildRef<T>& ref) {
      if (found == nullptr) {
        found = (ref.node == target) ? &root_ : findParentIn(ref.node, target);
      }
    });
    return found;
  }

  TreeNode<T>* findParentIn(TreeNode<T>* node, TreeNode<T>* target) {
    TreeNode<T>* found = nullptr;
    node->children_.forEach([&found, node, target,
                             this](const ChildRef<T>& ref) {
      if (found == nullptr) {
        found = (ref.node == target) ? node : findParentIn(ref.node, target);
      }
    });
    return found;
  }

  TreeNode<T> root_;  // 哨兵根：空树时它的值为 T()、没有儿女
  unsigned int size_;
};

// ============================== 哈希表 ==============================
// 通用哈希表：开放定址 + 线性探测，装载因子超过 0.7 就扩容。
// 约定：K() 被当作"空槽"标记，所以键不能等于 K()
// （姓名都是非空字符串，满足这个条件）。
// 删除用"墓碑"标记（Entry 里一个单独的 bool）：被删的槽位不能直接清空，
// 否则会截断线性探测的探测链，让后面的键查不到；墓碑太多时原地重建一次，
// 摊还下来仍是 O(1)。
template <typename K, typename V>
class HashMap {
 public:
  HashMap() {
    cap_ = 16;
    size_ = 0;
    tombstones_ = 0;
    slots_ = new Entry[cap_]();
  }

  HashMap(const HashMap&) = delete;  // 内部有裸指针，禁止拷贝
  HashMap& operator=(const HashMap&) = delete;
  ~HashMap() { delete[] slots_; }

  void clear() {
    for (unsigned int i = 0; i < cap_; i++) slots_[i] = Entry();
    size_ = 0;
    tombstones_ = 0;
  }

  // 按关键字查找：找到返回值的指针（可以直接改），找不到返回 nullptr
  // （K() 是"空槽/墓碑"的保留值，不作为真实键，直接当查不到）
  V* find(const K& key) {
    if (isEmptyKey(key)) return nullptr;
    unsigned int i = hashValue(key) & (cap_ - 1);
    while (!emptyAt(i)) {
      if (slots_[i].key == key) return &slots_[i].value;
      i = (i + 1) & (cap_ - 1);
    }
    return nullptr;
  }

  // 插入新的键值对；键已经存在时什么都不做
  void insert(const K& key, const V& value) {
    if (isEmptyKey(key) || find(key) != nullptr) return;
    if ((size_ + tombstones_ + 1) * 10 >= cap_ * 7) grow();
    place(key, value);
  }

  // 删除一个键：槽位改成"墓碑"（用单独的标记位记，不能改 key 的字节，
  // 否则以后复用这个槽位时那个字节会把新键首字节清零，键就查不到了）。
  // 保留墓碑是为了不截断线性探测的探测链：探测时墓碑要继续往后走。
  bool erase(const K& key) {
    if (isEmptyKey(key)) return false;  // K() 不是真实键，别把墓碑当命中
    unsigned int i = hashValue(key) & (cap_ - 1);
    while (!emptyAt(i)) {
      if (slots_[i].key == key) {
        slots_[i] = Entry();
        slots_[i].tombstone = true;
        size_--;
        tombstones_++;
        return true;
      }
      i = (i + 1) & (cap_ - 1);
    }
    return false;
  }

  unsigned int size() const { return size_; }

 private:
  struct Entry {
    K key;
    V value;
    bool tombstone;
    Entry() : key(), value(), tombstone(false) {}
  };

  // 键本身是不是"空键"（K() 被保留作空槽标记，不能真的当键用）
  static bool isEmptyKey(const K& key) {
    const unsigned char* bytes = (const unsigned char*)&key;
    for (unsigned int i = 0; i < sizeof(K); i++)
      if (bytes[i] != 0) return false;
    return true;
  }

  // 槽位 i 是不是真空槽：墓碑不算空（探测链必须穿过墓碑）
  bool emptyAt(const unsigned int i) const {
    return !slots_[i].tombstone && isEmptyKey(slots_[i].key);
  }

  // FNV-1a：逐字节混合，无填充字节的可平凡复制类型都能用
  static unsigned int hashValue(const K& key) {
    const unsigned char* bytes = (const unsigned char*)&key;
    unsigned int hash = 2166136261u;
    for (unsigned int i = 0; i < sizeof(K); i++)
      hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
  }

  // 找第一个真正的空槽
  unsigned int emptySlot(unsigned int i) const {
    while (!emptyAt(i)) i = (i + 1) & (cap_ - 1);
    return i;
  }

  void place(const K& key, const V& value) {
    const unsigned int i = emptySlot(hashValue(key) & (cap_ - 1));
    slots_[i].key = key;
    slots_[i].value = value;
    slots_[i].tombstone = false;
    size_++;
  }

  // 装载（含墓碑）超过 0.7：墓碑多就原地重建，否则容量翻倍
  void grow() {
    if (tombstones_ * 2 > size_) {
      rebuild(cap_);
    } else {
      rebuild(cap_ * 2);
    }
  }

  // 把所有活着的键值对重新放进 newCap 大小的新表（墓碑被丢掉）
  void rebuild(const unsigned int newCap) {
    Entry* old = slots_;
    const unsigned int oldCap = cap_;
    slots_ = new Entry[newCap]();
    cap_ = newCap;
    size_ = 0;
    tombstones_ = 0;
    for (unsigned int i = 0; i < oldCap; i++)
      if (!old[i].tombstone && !isEmptyKey(old[i].key))
        place(old[i].key, old[i].value);
    delete[] old;
  }

  Entry* slots_;
  unsigned int cap_;
  unsigned int size_;
  unsigned int tombstones_;
};

// ============================== 控制台输入 ==============================
class Input {
 public:
  // 读一个 [min, max] 范围内的整数，输入不合法会提示并要求重输
  static unsigned int readNumber(const char* prompt, const unsigned int min,
                                 const unsigned int max) {
    char line[kLineLen + 1];
    unsigned int value = 0;
    while (true) {
      std::cout << prompt;
      if (!readLine(line, kLineLen)) continue;
      if (!toUnsigned(line, value))
        std::cout << "输入不合法，请输入数字！" << '\n';
      else if (value < min || value > max)
        std::cout << "输入不合法，应输入 " << min << "~" << max << " 之间的数！"
                  << '\n';
      else
        return value;
    }
  }

  // 读一个不含空白的字段（姓名、报考类别）；out 的容量至少 maxLen + 1
  static void readWord(const char* prompt, const char* fieldName,
                       const unsigned int maxLen, char* out) {
    char line[kLineLen + 1];
    while (true) {
      std::cout << prompt;
      if (!readLine(line, kLineLen)) continue;

      // 掐掉首尾空白，得到 [begin, end)
      unsigned int begin = 0;
      unsigned int end = (unsigned int)strlen(line);
      while (begin < end && isBlank(line[begin])) begin++;
      while (end > begin && isBlank(line[end - 1])) end--;

      if (begin == end) {
        std::cout << "输入不合法，" << fieldName << "不能为空，请重新输入！"
                  << '\n';
      } else if (hasBlank(line + begin, end - begin)) {
        std::cout << "输入不合法，" << fieldName << "不能包含空格，请重新输入！"
                  << '\n';
      } else if (end - begin > maxLen) {
        std::cout << "输入不合法，" << fieldName << "最长 " << maxLen
                  << " 字节，请重新输入！" << '\n';
      } else {
        line[end] = '\0';
        strcpy(out, line + begin);
        return;
      }
    }
  }

  // 读一行并按空白切成若干字段：返回字段个数（0 表示空行）；
  // 返回 -1 表示这一行过长（提示已打印），调用者应重新读一次
  static int readFields(const char* prompt, char* line, char* fields[],
                        const unsigned int maxFields, const unsigned int cap) {
    std::cout << prompt;
    if (!readLine(line, cap)) return -1;
    return splitFields(line, fields, maxFields);
  }

  // 读一个菜单字母：只接受 letters 里出现过的字母，大小写不敏感
  static char readLetter(const char* prompt, const char* letters) {
    char line[kLineLen + 1];
    while (true) {
      std::cout << prompt;
      if (!readLine(line, kLineLen)) continue;
      unsigned int begin = 0;
      unsigned int end = (unsigned int)strlen(line);
      while (begin < end && isBlank(line[begin])) begin++;
      while (end > begin && isBlank(line[end - 1])) end--;
      if (end - begin == 1) {
        char letter = line[begin];
        if (letter >= 'a' && letter <= 'z') letter = (char)(letter - 'a' + 'A');
        for (unsigned int i = 0; letters[i] != '\0'; i++)
          if (letters[i] == letter) return letter;
      }
      std::cout << "输入不合法，请输入 " << letters << " 中的一个字母！"
                << '\n';
    }
  }

 private:
  static constexpr unsigned int kLineLen = 128;  // 一行最多读 128 个字符

  static bool isBlank(const char c) { return c == ' ' || c == '\t'; }

  static bool hasBlank(const char* text, const unsigned int len) {
    for (unsigned int i = 0; i < len; i++)
      if (isBlank(text[i])) return true;
    return false;
  }

  // 读一行；太长时提示并清掉本行剩余字符；输入结束（Ctrl+Z）直接退出程序
  static bool readLine(char* line, const unsigned int cap) {
    std::cin.getline(line, cap + 1);
    if (!std::cin.fail()) return true;
    if (std::cin.eof()) {
      std::cout << '\n' << "输入结束，程序退出!" << '\n';
      std::exit(0);
    }
    std::cout << "输入过长，请重新输入！" << '\n';
    std::cin.clear();
    char c;
    while (std::cin.get(c) && c != '\n') {
    }
    return false;
  }

  // 把一行按空白切成若干字段，返回字段个数；fields 依次指向各字段起点
  static int splitFields(char* line, char* fields[],
                         const unsigned int maxFields) {
    int count = 0;
    unsigned int i = 0;
    while (true) {
      while (isBlank(line[i])) i++;  // 跳过字段之间的空白
      if (line[i] == '\0') return count;
      if ((unsigned int)count == maxFields) return count + 1;  // 字段太多
      fields[count++] = line + i;                              // 记录字段起点
      while (line[i] != '\0' && !isBlank(line[i])) i++;
      if (line[i] == '\0') return count;
      line[i++] = '\0';  // 字段结尾，继续找下一个字段
    }
  }

  // 把字符串解析成 unsigned int：首尾空白忽略，逐位判界防止回绕
  static bool toUnsigned(const char* text, unsigned int& out) {
    unsigned int i = 0;
    while (isBlank(text[i])) i++;
    if (text[i] == '\0') return false;  // 空输入

    const unsigned long long kUintMax = 0xFFFFFFFFULL;
    unsigned long long value = 0;
    while (text[i] >= '0' && text[i] <= '9') {
      const unsigned long long digit = (unsigned long long)(text[i] - '0');
      if (value > (kUintMax - digit) / 10) return false;  // 再加一位就越界
      value = value * 10 + digit;
      i++;
    }
    while (isBlank(text[i])) i++;
    if (text[i] != '\0') return false;  // 数字后面还有别的东西
    out = (unsigned int)value;
    return true;
  }
};

// ============================== 家族成员 ==============================
// 家谱树里每个结点保存一个成员：姓名 ＋ 父亲（建家谱时先录入的一方）。
// 儿女关系不放在成员里，而是交给树 Tree<Member*> 的结点结构去表达。
class Member {
 public:
  Member() {
    name_[0] = '\0';
    parent_ = nullptr;
  }

  // 写入的姓名必须已经按 kNameLen 校验过长度，因此这里可以直接用 strcpy
  explicit Member(const char* name) {
    strcpy(name_, name);
    parent_ = nullptr;
  }

  const char* getName() const { return name_; }
  void setName(const char* name) { strcpy(name_, name); }
  Member* getParent() const { return parent_; }
  void setParent(Member* parent) { parent_ = parent; }

 private:
  char name_[kNameLen + 1];
  Member* parent_;
};

// ============================== 家谱 ==============================
// 家谱是一棵一般树 Tree<Member*>：根是祖先，每个成员结点有多个儿女结点。
// 另外用哈希表按姓名定位成员结点，避免每次操作都遍历整棵树。
class FamilyTree {
 public:
  FamilyTree() { memberName_[0] = '\0'; }

  // 析构：树里的 Member 是 new 出来的，Tree 只管结点，
  // 所以先遍历一遍把每个成员删掉，再让 ~Tree 释放所有结点
  ~FamilyTree() {
    tree_.forEachPreOrder([](Member* member) { delete member; });
  }

  // 打印菜单：菜单框只在程序开始时打印一次。
  // 每行的内容直接按题目示例写成字面量（宽度 51 列），
  // 只把"分隔线"用一条 nullptr 标出来，避免重复写两串等号。
  static void printMenu() {
    static const char* const rows[] = {
        "**                 家谱管理系统                  **",
        nullptr,  // 分隔线
        "**             请选择要执行的操作：              **",
        "**            A --- 完善家谱                     **",
        "**            B --- 添加家庭成员                 **",
        "**            C --- 解散局部家庭                 **",
        "**            D --- 更改家庭成员姓名             **",
        "**            E --- 退出程序                     **",
        nullptr,  // 分隔线
    };
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
      if (rows[i] == nullptr)
        printSeparator();
      else
        std::cout << rows[i] << '\n';
    }
  }

  // 首先建立一个家谱：只录入祖先，儿女后面再用 A 或 B 添加
  void buildFamily() {
    std::cout << "首先建立一个家谱！" << '\n';
    Input::readWord("请输入祖先的姓名： ", "姓名", kNameLen, memberName_);
    Member* ancestor = new Member(memberName_);
    tree_.setRoot(ancestor);  // 树里已经有一个哨兵根结点，改个值即可
    index_.insert(nameKey(memberName_), tree_.root());
    std::cout << "此家谱的祖先是：" << ancestor->getName() << '\n';
  }

  // A：完善家谱——给某个成员一次录入若干儿女
  void completeFamily() {
    TreeNode<Member*>* node = readNode("请输入要建立家庭的人的姓名： ");
    std::cout << "请输入" << node->value()->getName() << "的儿女人数：";
    const unsigned int count = Input::readNumber("", 0, kMaxChildCount);
    if (count == 0) {
      printChildren(node);
      return;
    }
    if (!hasRoomFor(count)) {
      printChildren(node);
      return;
    }
    if (node->children().size() + count > kMaxChildCount) {
      std::cout << "输入不合法，" << node->value()->getName()
                << "的儿女数已超过上限 " << kMaxChildCount << "！" << '\n';
      printChildren(node);
      return;
    }
    readChildren(node, count);
    printChildren(node);
  }

  // B：添加家庭成员——给某个成员添加一个儿子（或女儿）
  void addMember() {
    TreeNode<Member*>* node =
        readNode("请输入要添加儿子（或女儿）的人的姓名： ");
    if (node->children().size() >= kMaxChildCount) {
      std::cout << "输入不合法，" << node->value()->getName()
                << "的儿女数已达上限 " << kMaxChildCount << "！" << '\n';
      return;
    }
    if (!hasRoomFor(1)) return;
    char name[kNameLen + 1];
    while (true) {
      std::cout << "请输入" << node->value()->getName()
                << "新添加的儿子（或女儿）的姓名： ";
      Input::readWord("", "姓名", kNameLen, name);
      if (!hasName(name)) break;
      std::cout << "输入不合法，姓名" << name << "已存在，请重新输入！" << '\n';
    }
    Member* child = new Member(name);
    child->setParent(node->value());
    TreeNode<Member*>* childNode = tree_.addChild(*node, child);
    index_.insert(nameKey(name), childNode);  // 增量维护：只加这一个键
    printChildren(node);
  }

  // C：解散局部家庭——把该成员连同他的全部子孙从家谱里删掉
  void dissolveFamily() {
    TreeNode<Member*>* node = readNode("请输入要解散家庭的人的姓名： ");
    if (node == tree_.root()) {
      std::cout << "输入不合法，祖先不能解散，请重新输入！" << '\n';
      return;
    }
    std::cout << "要解散家庭的人是： " << node->value()->getName() << '\n';
    printChildren(node);
    removeSubtree(node);
  }

  // D：更改家庭成员姓名
  void renameMember() {
    TreeNode<Member*>* node = readNode("请输入要更改姓名的人的目前姓名： ");
    char oldName[kNameLen + 1];
    strcpy(oldName, node->value()->getName());
    char name[kNameLen + 1];
    while (true) {
      Input::readWord("请输入更改后的姓名： ", "姓名", kNameLen, name);
      if (strcmp(name, oldName) == 0) {
        std::cout << "输入不合法，新姓名与原名相同，请重新输入！" << '\n';
        continue;
      }
      if (hasName(name)) {
        std::cout << "输入不合法，姓名" << name << "已存在，请重新输入！"
                  << '\n';
        continue;
      }
      break;
    }
    Member* member = node->value();
    index_.erase(nameKey(oldName));
    member->setName(name);
    tree_.setNode(node, member);
    index_.insert(nameKey(name), node);
    std::cout << oldName << "已更名为" << name << '\n';
  }

 private:
  // 姓名做哈希表的键：整个键先清零再写字符串，保证哈希值稳定
  struct NameKey {
    char name[kNameLen + 1];
    bool operator==(const NameKey& other) const {
      return strcmp(name, other.name) == 0;
    }
  };

  // 读一行里的 count 个儿女姓名：姓名必须齐全、不超长、不与家谱中已有姓名重复
  void readChildren(TreeNode<Member*>* node, const unsigned int count) {
    if (!hasRoomFor(count)) return;  // 人数上限先查一次，不在重输循环里反复提示
    char line[kNameLineLen + 1];
    char* fields[kMaxChildCount + 2];
    while (true) {
      std::cout << "请依次输入" << node->value()->getName()
                << "的儿女的姓名： ";
      const int fieldCount =
          Input::readFields("", line, fields, kMaxChildCount + 1, kNameLineLen);
      if (fieldCount < 0) continue;  // 这一行太长，提示已经打印过
      if (fieldCount != (int)count) {
        std::cout << "输入不合法，应有 " << count << " 个姓名，请重新输入！"
                  << '\n';
        continue;
      }
      bool valid = true;
      for (int i = 0; i < fieldCount && valid; i++) {
        if (strlen(fields[i]) > kNameLen) {
          std::cout << "输入不合法，姓名最长 " << kNameLen
                    << " 字节，请重新输入！" << '\n';
          valid = false;
          break;
        }
        for (int j = 0; j < i && valid; j++)
          if (strcmp(fields[i], fields[j]) == 0) {
            std::cout << "输入不合法，姓名" << fields[i]
                      << "重复出现，请重新输入！" << '\n';
            valid = false;
          }
        if (valid && hasName(fields[i])) {
          std::cout << "输入不合法，姓名" << fields[i] << "已存在，请重新输入！"
                    << '\n';
          valid = false;
        }
      }
      if (!valid) continue;

      for (int i = 0; i < fieldCount; i++) {
        Member* child = new Member(fields[i]);
        child->setParent(node->value());
        TreeNode<Member*>* childNode = tree_.addChild(*node, child);
        index_.insert(nameKey(fields[i]), childNode);  // 增量维护：边挂边记
      }
      return;
    }
  }

  // 读一个家谱里已有的成员姓名，找不到就提示并重新输入
  TreeNode<Member*>* readNode(const char* prompt) {
    char name[kNameLen + 1];
    while (true) {
      Input::readWord(prompt, "姓名", kNameLen, name);
      TreeNode<Member*>* node = find(name);
      if (node != nullptr) return node;
      std::cout << "输入不合法，家谱中没有叫" << name << "的成员，请重新输入！"
                << '\n';
    }
  }

  // 打印某个成员的第一代子孙（树里就是他的儿女结点），姓名之间用制表符分开
  void printChildren(TreeNode<Member*>* node) const {
    std::cout << node->value()->getName() << "的第一代子孙是： ";
    if (node->children().size() == 0) {
      std::cout << "（无）" << '\n';
      return;
    }
    node->children().forEach([](const ChildRef<Member*>& ref) {
      std::cout << '\t' << ref.node->value()->getName();
    });
    std::cout << '\n';
  }

  // 按姓名查找成员结点：查哈希表，平均 O(1)，索引里存的就是树结点指针
  TreeNode<Member*>* find(const char* name) {
    TreeNode<Member*>** found = index_.find(nameKey(name));
    return found == nullptr ? nullptr : *found;
  }

  bool hasName(const char* name) { return find(name) != nullptr; }

  // 还能不能再加 count 个成员：家谱总人数有 kMaxMemberCount 的上限
  bool hasRoomFor(const unsigned int count) const {
    if (tree_.size() + count <= kMaxMemberCount) return true;
    std::cout << "输入不合法，家谱成员数已达上限 " << kMaxMemberCount << "！"
              << '\n';
    return false;
  }

  // 删除以 node 为根的整棵子树。
  // 先按前序把这棵子树里的每个成员处理掉（先摘索引、再释放 Member），
  // 最后交给 Tree::eraseNode 释放这些结点——结点析构会把整棵子树带走。
  void removeSubtree(TreeNode<Member*>* node) {
    if (node == nullptr || node == tree_.root()) return;  // 祖先不能解散
    Member* parent = node->value()->getParent();
    // 父结点用成员自己的 parent_ 指针直接定位（名字全局唯一），不遍历整棵树
    TreeNode<Member*>* parentNode =
        (parent == nullptr) ? nullptr : find(parent->getName());
    if (parentNode == nullptr) return;

    tree_.forEachInSubtree(node, [this](TreeNode<Member*>* p) {
      index_.erase(nameKey(p->value()->getName()));
      delete p->value();
    });
    tree_.eraseNode(parentNode, node);
  }

  // 姓名 → NameKey：先把键清零，保证哈希值稳定
  static NameKey nameKey(const char* name) {
    NameKey key = NameKey();
    strcpy(key.name, name);
    return key;
  }

  // 分隔线：整行 kMenuWidth 个等号
  static void printSeparator() {
    for (unsigned int i = 0; i < kMenuWidth; i++) std::cout << '=';
    std::cout << '\n';
  }

  char memberName_[kNameLen + 1];               // 录入姓名用的缓冲区
  Tree<Member*> tree_;                          // 家谱本体：一般树
  HashMap<NameKey, TreeNode<Member*>*> index_;  // 姓名 → 树结点，平均 O(1)
};
