// ============================================================================
//  题目二  约瑟夫生者死者游戏
//  文件结构：常量 → 单链表 List<T> → 单循环链表 CircularList<T>
//            → 控制台输入 Input → 约瑟夫游戏 JosephusGame
//  数据结构：带头结点的单循环链表（旅客序号围成一圈）＋ 带头结点的单链表（出局顺序）
//  复杂度：建环 O(n)；每出局一人最多走 min(M, 当前环长) 步并摘链 O(1)，
//          整局 O(n·min(M,n))，不再随 M 无谓增大（M 上限 1 亿也只走环长步）；空间 O(n)
//  说明：类与函数都定义在本文件里（成员函数直接在类里实现），T2.cpp 里只有主函数；
//        链表是自己写的，没有使用任何 STL 容器
// ============================================================================

#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>

// ============================== 常量 ==============================
const unsigned int kMinPassengerCount = 2;        // 旅客人数下限
const unsigned int kMaxPassengerCount = 100000;   // 旅客人数上限
const unsigned int kMinDeathNumber = 1;           // 死亡数字 M 的下限
const unsigned int kMaxDeathNumber = 100000000;   // 死亡数字 M 的上限（1 亿）
const unsigned int kMinRemainCount = 0;           // 剩余生者人数 K 的下限（0 = 全部出局）

// ============================== 单链表 ==============================
template <typename T>
struct ListNode {
  T data;
  ListNode<T>* next;
  explicit ListNode(const T& value) : data(value), next(nullptr) {}
};

// 带头结点的单链表：头结点（哨兵）不存数据，尾指针让尾插是 O(1)
template <typename T>
class List {
 public:
  List() {
    head_ = new ListNode<T>(T());
    tail_ = head_;
    size_ = 0;
  }

  // 深拷贝：结点是 new 出来的，两个链表不能共用结点，否则析构时会重复释放
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

  // 插入到第 pos 个位置（pos 从 1 开始，size() + 1 表示插到表尾）
  bool insert(unsigned int pos, const T& value) {
    if (pos == 0) return false;
    ListNode<T>* p = head_;
    for (unsigned int i = 1; i < pos; i++) {
      if (p->next == nullptr) return false;
      p = p->next;
    }
    ListNode<T>* node = new ListNode<T>(value);
    node->next = p->next;
    p->next = node;
    if (p == tail_) tail_ = node;
    size_++;
    return true;
  }

  // 一次遍历访问全部元素：输出、统计、重建索引都用它。
  // 参数func填对每个元素data操作的函数名，但要求func对List中元素只读
  template <typename Func>
  void forEach(Func func) const {
    for (const ListNode<T>* p = head_->next; p != nullptr; p = p->next)
      func(p->data);
  }

  // 找到第一个满足条件的结点（可以直接改它的数据），找不到返回 nullptr
  template <typename Func>
  ListNode<T>* findFirstNode(Func pred) {
    for (ListNode<T>* p = head_->next; p != nullptr; p = p->next)
      if (pred(p->data)) return p;
    return nullptr;
  }

  // 删除第一个满足条件的结点（找到后摘链是 O(1)）
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

  unsigned int size() const { return size_; }

 private:
  void copyFrom(const List& other) {
    for (const ListNode<T>* p = other.head_->next; p != nullptr; p = p->next)
      push_back(p->data);
  }

  ListNode<T>* head_;
  ListNode<T>* tail_;
  unsigned int size_;
};

// ============================== 单循环链表 ==============================
// 单循环链表结点：与单链表的结点相比只差“成环”，结点结构保持一致
template <typename T>
struct CircularNode {
  T data;
  CircularNode<T>* next;
  explicit CircularNode(const T& value) : data(value), next(nullptr) {}
};

// 带头结点的单循环链表：头结点（哨兵）不存数据，尾结点的 next 指回头结点，
// 因此从头结点出发沿 next 走 size 步就能绕回原处。游戏里的出局只改指针，
// 不需要搬移任何数据，摘链是 O(1)。
template <typename T>
class CircularList {
 public:
  CircularList() {
    head_ = new CircularNode<T>(T());
    head_->next = head_;
    tail_ = head_;
    size_ = 0;
  }

  CircularList(const CircularList&) = delete;  // 内部有裸指针，禁止拷贝
  CircularList& operator=(const CircularList&) = delete;
  ~CircularList() {
    clear();
    delete head_;
  }

  void clear() {
    CircularNode<T>* p = head_->next;
    while (p != nullptr && p != head_) {
      CircularNode<T>* next = p->next;
      delete p;
      p = next;
    }
    head_->next = head_;
    tail_ = head_;
    size_ = 0;
  }

  // 尾插：新结点接在尾结点之后，并让尾结点重新指回头结点
  void push_back(const T& value) {
    CircularNode<T>* node = new CircularNode<T>(value);
    node->next = head_;
    tail_->next = node;
    tail_ = node;
    size_++;
  }

  // 找到第 pos 个结点（pos 从 1 开始），超出范围返回 nullptr
  CircularNode<T>* nodeAt(const unsigned int pos) {
    if (pos == 0 || pos > size_) return nullptr;
    CircularNode<T>* p = head_->next;
    for (unsigned int i = 1; i < pos; i++) p = p->next;
    return p;
  }

  // 从 head_->next 开始绕着环访问每个结点，直到回到头结点
  template <typename Func>
  void forEach(Func func) const {
    if (head_->next == head_) return;
    const CircularNode<T>* p = head_->next;
    do {
      func(p->data);
      p = p->next;
    } while (p != head_);
  }

  // ---- 供外部算法（约瑟夫报数）在环上移动与摘链的接口 ----

  // 头结点（哨兵）：它的后继就是环上第 1 个结点，
  // 因此它可以直接充当“第 1 个人”的报数前驱
  CircularNode<T>* headNode() const { return head_; }

  // 沿环向后走 steps 步，返回走到的结点（steps = 0 时原样返回）
  CircularNode<T>* advance(CircularNode<T>* node,
                           const unsigned int steps) const {
    for (unsigned int i = 0; i < steps; i++) node = nextOf(node);
    return node;
  }

  // 摘掉 prev 的后继并返回它的数据：prev 可以是头结点（此时摘掉第 1 个结点），
  // 也可以是尾结点（此时摘掉第 1 个结点，因为尾结点的后继就是环上的第一个人）。
  // 摘链后 prev 的后继就是出局者的后继，即下一位报数者的前驱。
  // 环为空时没有可摘的结点（正常玩法走不到这里：调用点在 size() > remain >= 0 时），
  // 此时返回默认值并且不改动哨兵，避免误删哨兵造成堆损坏
  T eraseAfter(CircularNode<T>* prev) {
    if (size_ == 0) return T();
    CircularNode<T>* target = nextOf(prev);
    if (prev == target) {           // 环上只剩一个结点，摘掉后环就空了
      head_->next = head_;
      tail_ = head_;
    } else {
      if (prev == tail_)
        head_->next = target->next;  // prev 的后继是哨兵：要改的是哨兵的 next
      else
        prev->next = target->next;   // 摘链：前驱直接跳过出局者
      if (target == tail_) tail_ = prev;  // 出局的是尾结点，尾指针要回退
    }
    const T value = target->data;
    delete target;
    size_--;
    return value;
  }

  unsigned int size() const { return size_; }

 private:
  // 沿环向后走一步；哨兵不参加报数，走到哨兵就跳到第一个人
  CircularNode<T>* nextOf(const CircularNode<T>* node) const {
    CircularNode<T>* next = node->next;
    return next == head_ ? head_->next : next;
  }

  CircularNode<T>* head_;
  CircularNode<T>* tail_;
  unsigned int size_;
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
      fields[count++] = line + i;  // 记录字段起点
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

// ============================== 约瑟夫游戏 ==============================
// 旅客序号 1~N 存在单循环链表里，出局顺序存在单链表里，
// 游戏结束前不再为任何操作分配内存（除了建环时的 N 个结点）
class JosephusGame {
 public:
  JosephusGame() {
    total_ = 0;
    start_ = 0;
    deathNumber_ = 0;
    remain_ = 0;
  }

  // 打印游戏规则
  void printRule() const {
    std::cout << "现有N人围成一圈，从第S个人开始依次报数，报M的人出局，再由下一人开始报"
                 "数，如此循环，直至剩下K个人为止"
              << '\n';
    std::cout << '\n';
  }

  // 依次读入 N、S、M、K，并保证取值范围合法
  //（K 允许取 0：所有人都要出局，最后无人存活）
  void readParameters() {
    total_ = Input::readNumber("请输入生死游戏的总人数N：", kMinPassengerCount,
                               kMaxPassengerCount);
    start_ = Input::readNumber("请输入游戏开始的位置S：", 1, total_);
    deathNumber_ =
        Input::readNumber("请输入死亡数字M：", kMinDeathNumber, kMaxDeathNumber);
    remain_ =
        Input::readNumber("请输入剩余的生者人数K：", kMinRemainCount, total_);
  }

  // 把 1~N 号旅客尾插成环
  void buildRing() {
    passengers_.clear();
    dead_.clear();
    for (unsigned int i = 1; i <= total_; i++) passengers_.push_back(i);
  }

  // 开始游戏：每报到第 deathNumber_ 个人就出局，直到剩下 remain_ 个人
  void play() { runJosephus(start_, deathNumber_, remain_, dead_); }

  // 约瑟夫报数：从第 start 号开始依次报数，每报到第 step 个人就出局，
  // 直到剩下 remain 个人为止（remain = 0 表示全部出局）；出局者的编号按顺序追加到 dead 里。
  // 优化：报数绕整圈会回到原处，所以每轮先把 step 对当前环长取模，
  //       单次出局的步数从 step 降到 min(step, 当前环长)，
  //       M 远大于 N 时不再一圈一圈空跑，整局 O(n·min(M, n))。
  // prev 始终指向“下一个要报数的人”的前一个结点。
  void runJosephus(const unsigned int start, const unsigned int step,
                   const unsigned int remain, List<unsigned int>& dead) {
    if (passengers_.size() <= remain) return;  // 人数已经满足要求，无人出局
    CircularNode<unsigned int>* prev =
        start == 1 ? passengers_.headNode() : passengers_.nodeAt(start - 1);
    if (prev == nullptr) prev = passengers_.headNode();  // 位置越界时从头结点开始（调用者已保证 start 合法）
    while (passengers_.size() > remain) {
      // 整圈步数等于回到原处，先去整圈：effective 落在 [1, 当前环长]
      const unsigned int effective = (step - 1) % passengers_.size() + 1;
      prev = passengers_.advance(prev, effective - 1);  // 走到出局者的前驱
      // 摘链 O(1)，出局后 prev 的后继就是下一位报数者的前驱
      dead.push_back(passengers_.eraseAfter(prev));
    }
  }

  // 按出局顺序打印每个死者的位置
  void printDead() const {
    if (dead_.size() == 0) {
      std::cout << "没有人出局！" << '\n';
      return;
    }
    unsigned int index = 0;
    dead_.forEach([&index](const unsigned int position) {
      index++;
      std::cout << "第" << index << "个死者的位置是：\t" << position << '\n';
    });
  }

  // 打印剩余人数和剩余旅客的位置（沿着环从头结点后面依次输出，序号是递增的）；
  // K = 0 时环上已经没有结点，给出明确的占位提示，不会输出空行
  void printSurvivors() const {
    std::cout << "最后剩下：\t" << remain_ << "人" << '\n';
    std::cout << "剩余的生存者位置为：";
    if (passengers_.size() == 0) {
      std::cout << "\t（没有人存活）";
    } else {
      passengers_.forEach([](const unsigned int position) {
        std::cout << '\t' << position;
      });
    }
    std::cout << '\n';
  }

 private:
  unsigned int total_;        // 旅客总数 N
  unsigned int start_;        // 开始报数的位置 S
  unsigned int deathNumber_;  // 死亡数字 M
  unsigned int remain_;       // 剩余的生者人数 K
  CircularList<unsigned int> passengers_;  // 单循环链表：旅客围成一圈
  List<unsigned int> dead_;                // 单链表：出局顺序
};
