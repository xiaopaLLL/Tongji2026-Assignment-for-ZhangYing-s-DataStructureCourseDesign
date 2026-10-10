// ============================================================================
//  题目一  考试报名系统
//  文件结构：常量 → 单链表 List<T> → 考生信息 Student → 哈希表 HashMap<K, V>
//            → 控制台输入 Input → 考试报名系统 StudentSystem
//  数据结构：带头结点、带尾指针的单链表（存放考生信息）
//            ＋ 哈希索引（考号 → 该考生结点的**前驱**，兼作考号判重）
//  复杂度：按考号查找 / 删除 / 修改考号 / 判重 平均 O(1)
//          （索引拿到前驱 → 前驱的 next 就是考生结点 → 改前驱的 next 即摘链）
//          按位置插入 O(n)；整表操作（输出、统计）一次遍历 O(n)；尾插 O(1)
//  边界：考生人数上限 kMaxStudentCount，达到上限后 add / insert 一律拒绝插入
//  说明：类与函数都定义在本文件里（成员函数直接在类里实现），T1.cpp 里只有主函数；
//        链表和哈希表都是自己写的，没有使用任何 STL 容器
// ============================================================================
#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>

// ============================== 常量 ==============================
const unsigned int kMaxStudentCount = 1000000;  // 考生人数上限
const unsigned int kMaxNumber = 1000000000;     // 考号上限
const unsigned int kMinAge = 1;                 // 年龄下限
const unsigned int kMaxAge = 200;               // 年龄上限
const unsigned int kNumberLen = 16;             // 考号最多 16 位
const unsigned int kNameLen = 20;               // 姓名最多 20 字节
const unsigned int kGenderLen = 6;              // 性别：男 / 女 / 其它
const unsigned int kAgeLen = 3;                 // 年龄最多 3 位（1~200）
const unsigned int kTypeLen = 20;               // 报考类别最多 20 字节

// ============================== 单链表 ==============================
template <typename T>
struct ListNode {
  T data;
  ListNode<T>* next;
  explicit ListNode(const T& value) : data(value), next(nullptr) {}
};

// 带头结点、带尾指针的单链表：头结点（哨兵）不存数据，尾指针让尾插是 O(1)。
// 结点里没有前驱指针，所以“摘链”必须由调用者递上前驱结点——
// 本项目里这个前驱就存在哈希索引里（考号 → 前驱结点），查找取前驱的 next。
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

  // 尾插，返回新结点。新结点的前驱就是插入前的尾结点（用 tail() 先把前驱取走）
  ListNode<T>* push_back(const T& value) {
    ListNode<T>* node = new ListNode<T>(value);
    tail_->next = node;
    tail_ = node;
    size_++;
    return node;
  }

  // 在 prev 之后插入一个结点（prev 必须是本链表中的结点，可以是头结点），返回新结点
  ListNode<T>* insertAfter(ListNode<T>* prev, const T& value) {
    ListNode<T>* node = new ListNode<T>(value);
    node->next = prev->next;
    prev->next = node;
    if (prev == tail_) tail_ = node;  // 插到表尾要更新尾指针
    size_++;
    return node;
  }

  // 取第 index 个结点（index 从 0 开始，0 表示头结点）；越界返回 nullptr。
  // 按位置插入时用它走到 pos-1，那个结点就是新结点的前驱
  ListNode<T>* nodeAt(unsigned int index) {
    ListNode<T>* p = head_;
    for (unsigned int i = 0; i < index; i++) {
      if (p->next == nullptr) return nullptr;
      p = p->next;
    }
    return p;
  }

  // 已知前驱时摘链删除：改前驱的 next + 释放结点，O(1)。
  // prev 为空或它后面没有结点时返回 false，不做任何改动
  bool eraseAfter(ListNode<T>* prev) {
    if (prev == nullptr || prev->next == nullptr) return false;
    ListNode<T>* node = prev->next;
    prev->next = node->next;
    if (node == tail_) tail_ = prev;  // 删的是尾结点，尾指针回退
    delete node;
    size_--;
    return true;
  }

  // 尾结点（空表时就是头结点）：尾插之前先取它，它就是新结点的前驱
  ListNode<T>* tail() const { return tail_; }

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

  // 删除第一个满足条件的结点：遍历时一直握着前驱，命中就 O(1) 摘链
  template <typename Func>
  bool eraseFirst(Func pred) {
    for (ListNode<T>* p = head_; p->next != nullptr; p = p->next)
      if (pred(p->next->data)) return eraseAfter(p);
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

// ============================== 考生信息 ==============================
class Student {
 public:
  Student() {
    number_ = 0;
    age_ = 0;
    name_[0] = gender_[0] = type_[0] = '\0';
  }

  // 写入的字符串都必须先按字段上限校验过长度，因此这里可以直接用 strcpy
  Student(const unsigned int number, const char* name, const char* gender,
          const unsigned int age, const char* type) {
    number_ = number;
    age_ = age;
    strcpy(name_, name);
    strcpy(gender_, gender);
    strcpy(type_, type);
  }

  unsigned int getNumber() const { return number_; }
  const char* getGender() const { return gender_; }
  unsigned int getAge() const { return age_; }
  const char* getType() const { return type_; }

  void setNumber(const unsigned int number) { number_ = number; }
  void setName(const char* name) { strcpy(name_, name); }
  void setGender(const char* gender) { strcpy(gender_, gender); }
  void setAge(const unsigned int age) { age_ = age; }
  void setType(const char* type) { strcpy(type_, type); }

  // 打印一条考生信息，字段之间用制表符分隔
  void print() const {
    std::cout << number_ << '\t' << name_ << '\t' << gender_ << '\t' << age_
              << '\t' << type_ << '\n';
  }

  // 表头，字段顺序与 print() 一致
  static void printHeader() {
    std::cout << "考号\t姓名\t性别\t年龄\t报考类别" << '\n';
  }

 private:
  unsigned int number_;
  char name_[kNameLen + 1];
  char gender_[kGenderLen + 1];
  unsigned int age_;
  char type_[kTypeLen + 1];
};

// ============================== 哈希表 ==============================
// 通用哈希表：开放定址 + 线性探测，装载因子超过 0.7 就扩容。
// 约定：K() 被当作"空槽"标记，所以键不能等于 K()
//（考号都 >= 1、报考类别都是非空字符串，都满足这个条件）。
// 查找、插入、删除都是平均 O(1)：删除用“向后搬移”填补空洞，
// 不引入墓碑，因此删除之后查找也不会退化。
template <typename K, typename V>
class HashMap {
 public:
  HashMap() {
    cap_ = 16;
    size_ = 0;
    slots_ = new Entry[cap_]();
  }

  HashMap(const HashMap&) = delete;  // 内部有裸指针，禁止拷贝
  HashMap& operator=(const HashMap&) = delete;
  ~HashMap() { delete[] slots_; }

  void clear() {
    for (unsigned int i = 0; i < cap_; i++) slots_[i] = Entry();
    size_ = 0;
  }

  // 按关键字查找：找到返回值的指针（可以直接改），找不到返回 nullptr
  V* find(const K& key) {
    unsigned int i = hashValue(key) & (cap_ - 1);
    while (!isEmpty(slots_[i].key)) {
      if (slots_[i].key == key) return &slots_[i].value;
      i = (i + 1) & (cap_ - 1);
    }
    return nullptr;
  }

  // 插入新的键值对；插入成功返回 true，键已经存在返回 false（不覆盖旧值）
  bool insert(const K& key, const V& value) {
    if (isEmpty(key) || find(key) != nullptr) return false;
    if ((size_ + 1) * 10 >= cap_ * 7) resize(cap_ * 2);
    place(key, value);
    return true;
  }

  // 按键删除，删掉了返回 true，键不存在返回 false。平均 O(1)。
  // 线性探测下不能简单地把槽清空（会把探测链截断），这里用“向后搬移”：
  // 把同一簇里、本来也能落在空洞位置上的元素依次前移，最后清空末尾那个槽。
  // 这样既没有墓碑，也不会让查找变慢。
  bool erase(const K& key) {
    unsigned int hole = hashValue(key) & (cap_ - 1);
    while (!isEmpty(slots_[hole].key)) {
      if (slots_[hole].key == key) break;
      hole = (hole + 1) & (cap_ - 1);
    }
    if (isEmpty(slots_[hole].key)) return false;  // 键不存在

    unsigned int j = hole;
    while (true) {
      j = (j + 1) & (cap_ - 1);
      if (isEmpty(slots_[j].key)) break;
      const unsigned int home = hashValue(slots_[j].key) & (cap_ - 1);
      // home 落在探测区间 (hole, j] 内才需要前移；否则它本就不该待在 j
      const bool inside = (j > hole) ? (home > hole && home <= j)
                                     : (home > hole || home <= j);
      if (!inside) {
        slots_[hole] = slots_[j];
        hole = j;
      }
    }
    slots_[hole] = Entry();
    size_--;
    return true;
  }

  unsigned int size() const { return size_; }

 private:
  struct Entry {
    K key;
    V value;
  };

  static bool isEmpty(const K& key) { return key == K(); }

  // FNV-1a：逐字节混合，无填充字节的可平凡复制类型都能用
  static unsigned int hashValue(const K& key) {
    const unsigned char* bytes = (const unsigned char*)&key;
    unsigned int hash = 2166136261u;
    for (unsigned int i = 0; i < sizeof(K); i++)
      hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
  }

  void place(const K& key, const V& value) {
    unsigned int i = hashValue(key) & (cap_ - 1);
    while (!isEmpty(slots_[i].key)) i = (i + 1) & (cap_ - 1);
    slots_[i].key = key;
    slots_[i].value = value;
    size_++;
  }

  void resize(const unsigned int newCap) {
    Entry* old = slots_;
    const unsigned int oldCap = cap_;
    slots_ = new Entry[newCap]();
    cap_ = newCap;
    size_ = 0;
    for (unsigned int i = 0; i < oldCap; i++)
      if (!isEmpty(old[i].key)) place(old[i].key, old[i].value);
    delete[] old;
  }

  Entry* slots_;
  unsigned int cap_;
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

  // 读性别，只接受 男 / 女 / 其它
  static void readGender(const char* prompt, char* out) {
    while (true) {
      readWord(prompt, "性别", kGenderLen, out);
      if (strcmp(out, "男") == 0 || strcmp(out, "女") == 0 ||
          strcmp(out, "其它") == 0)
        return;
      std::cout << "输入不合法，性别只能是 男/女/其它，请重新输入！" << '\n';
    }
  }

  // 读一条考生信息：一行内用空格分隔 5 个字段
  static Student readStudent(const char* prompt) {
    char line[kLineLen + 1];
    while (true) {
      std::cout << prompt;
      if (!readLine(line, kLineLen)) continue;

      char* field[5] = {nullptr};
      if (splitFields(line, field, 5) != 5) {
        std::cout << "输入不合法，每行应有 5 个字段：考号 姓名 性别 年龄 报考类别！"
                  << '\n';
        continue;
      }

      unsigned int number = 0, age = 0;
      if (!toUnsigned(field[0], number) || number < 1 || number > kMaxNumber) {
        std::cout << "输入不合法，考号应为 1~" << kMaxNumber
                  << " 之间的整数！" << '\n';
        continue;
      }
      if (strlen(field[1]) > kNameLen) {
        std::cout << "输入不合法，姓名最长 " << kNameLen << " 字节！" << '\n';
        continue;
      }
      if (strcmp(field[2], "男") != 0 && strcmp(field[2], "女") != 0 &&
          strcmp(field[2], "其它") != 0) {
        std::cout << "输入不合法，性别只能是 男/女/其它！" << '\n';
        continue;
      }
      if (!toUnsigned(field[3], age) || age < kMinAge || age > kMaxAge) {
        std::cout << "输入不合法，年龄应为 " << kMinAge << "~" << kMaxAge
                  << " 之间的整数！" << '\n';
        continue;
      }
      if (strlen(field[4]) > kTypeLen) {
        std::cout << "输入不合法，报考类别最长 " << kTypeLen << " 字节！"
                  << '\n';
        continue;
      }
      return Student(number, field[1], field[2], age, field[4]);
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

// ============================== 考试报名系统 ==============================
// 考生信息存在单链表里，另外用哈希表做索引：考号 → 该考生结点的前驱。
// 因此按考号查找、删除、改考号、判重都是平均 O(1)，不再遍历链表；
// 单链表没有前驱指针，索引里存前驱正好补上这一块：摘链就是改前驱的 next。
// 索引与链表同步维护：插入、删除、改考号时各改一次索引，不需要重建。
class StudentSystem {
 public:
  // ---- 每个操作一个成员函数，主函数里直接 sys.xxx() 调用 ----

  // 建立考生信息系统：先读人数，再逐条录入
  void build() {
    std::cout << "首先请建立考生信息系统!" << '\n';
    const unsigned int count =
        Input::readNumber("请输入考生人数：", 1, kMaxStudentCount);
    std::cout << "请依次输入考生的考号，姓名，性别，年龄及报考类别!" << '\n';
    for (unsigned int i = 0; i < count; i++) {
      // count 已经被 readNumber 限制在 1 ~ kMaxStudentCount，且此时链表为空，
      // 正常情况下 add 不会失败；这里仍然判断返回值，超限就停下并提示
      if (!add(readNewStudent(""))) {
        printFullMessage();
        break;
      }
    }
    printAll();
  }

  // 插入：先判断人数是否已达上限，再读位置，最后读一条考生信息
  void addStudent() {
    if (isFull()) {  // 已达上限：直接拒绝，连位置与考生信息都不用读
      printFullMessage();
      return;
    }
    const unsigned int pos =
        Input::readNumber("请输入你要插入的考生的位置：", 1, size() + 1);
    const Student student = readNewStudent(
        "请依次输入要插入的考生的考号，姓名，性别，年龄及报考类别!\n");
    if (insert(pos, student))
      printAll();
    else
      std::cout << "插入失败，位置非法!" << '\n';
  }

  // 删除：按考号删除，并把被删除的考生信息打印出来
  void removeStudent() {
    const unsigned int number =
        Input::readNumber("请输入要删除的考生的考号：", 1, kMaxNumber);
    Student removed;
    if (!remove(number, removed)) {
      std::cout << "未找到考号为 " << number << " 的考生!" << '\n';
      return;
    }
    std::cout << "你删除的考生信息是：";
    removed.print();
    printAll();
  }

  // 查找：按考号查找并打印
  void findStudent() {
    const unsigned int number =
        Input::readNumber("请输入要查找的考生的考号：", 1, kMaxNumber);
    const Student* target = find(number);
    if (target == nullptr) {
      std::cout << "未找到考号为 " << number << " 的考生!" << '\n';
      return;
    }
    Student::printHeader();
    target->print();
  }

  // 修改：按考号定位后逐个字段修改，选 0 结束修改并输出整表
  void modifyStudent() {
    const unsigned int number =
        Input::readNumber("请输入要修改的考生的考号：", 1, kMaxNumber);
    ListNode<Student>* prev = findPrev(number);  // 索引里存的就是前驱，平均 O(1)
    if (prev == nullptr) {
      std::cout << "未找到考号为 " << number << " 的考生!" << '\n';
      return;
    }
    ListNode<Student>* node = prev->next;  // 考生结点 = 前驱的 next
    Student* target = &node->data;
    std::cout << "该考生的当前信息如下：" << '\n';
    Student::printHeader();
    target->print();

    while (true) {
      const unsigned int field = Input::readNumber(
          "请选择要修改的字段（1考号 2姓名 3性别 4年龄 5报考类别，0结束修改）：", 0,
          5);
      if (field == 0) {
        printAll();
        return;
      }
      if (field == 1) {
        const unsigned int newNumber =
            Input::readNumber("请输入新的考号：", 1, kMaxNumber);
        if (newNumber == target->getNumber()) {
          std::cout << "考号没有变化!" << '\n';
          continue;
        }
        if (exists(newNumber)) {
          std::cout << "考号 " << newNumber << " 已存在，修改失败!" << '\n';
          continue;
        }
        // 索引换个键就行：前驱没变，删掉旧考号、登记新考号，都是平均 O(1)
        index_.erase(target->getNumber());
        target->setNumber(newNumber);
        index_.insert(newNumber, prev);
      } else if (field == 2) {
        char name[kNameLen + 1];
        Input::readWord("请输入新的姓名：", "姓名", kNameLen, name);
        target->setName(name);
      } else if (field == 3) {
        char gender[kGenderLen + 1];
        Input::readGender("请输入新的性别（男/女/其它）：", gender);
        target->setGender(gender);
      } else if (field == 4) {
        target->setAge(Input::readNumber("请输入新的年龄：", kMinAge, kMaxAge));
      } else {
        char type[kTypeLen + 1];
        Input::readWord("请输入新的报考类别：", "报考类别", kTypeLen, type);
        target->setType(type);
      }
      std::cout << "修改后的考生信息：" << '\n';
      Student::printHeader();
      target->print();
    }
  }

  unsigned int size() const { return students_.size(); }

  // 考生人数是否已经达到上限（达到上限后不允许再插入）
  bool isFull() const { return students_.size() >= kMaxStudentCount; }

  bool exists(const unsigned int number) {
    return findPrev(number) != nullptr;
  }

  // 尾插一位考生，并把“考号 → 前驱结点”登记进哈希索引；
  // 人数已达上限或考号已存在时返回 false，链表与索引都保持不变
  bool add(const Student& student) {
    if (isFull()) return false;
    ListNode<Student>* prev = students_.tail();  // 新结点的前驱＝当前的尾结点
    if (!index_.insert(student.getNumber(), prev)) return false;  // 考号已存在
    students_.push_back(student);
    return true;
  }

  // 插入到第 pos 个位置并登记索引；
  // 人数已达上限、pos 非法或考号已存在时返回 false，链表与索引都保持不变
  bool insert(const unsigned int pos, const Student& student) {
    if (isFull()) return false;
    if (pos == 0 || pos > students_.size() + 1) return false;  // 位置越界
    ListNode<Student>* prev = students_.nodeAt(pos - 1);  // 走 pos-1 步，O(pos)
    ListNode<Student>* displaced = prev->next;  // 原来待在这个位置上的结点
    // 先登记索引：若考号重复会返回 false，此时链表一个指针都没动
    if (!index_.insert(student.getNumber(), prev)) return false;
    ListNode<Student>* node = students_.insertAfter(prev, student);
    if (displaced != nullptr) {
      // 被顶到后面的结点，它的前驱由 prev 变成了新结点
      const unsigned int number = displaced->data.getNumber();
      index_.erase(number);
      index_.insert(number, node);
    }
    return true;
  }

  // 按考号查找，返回可以直接修改的考生数据；找不到返回 nullptr。
  // 索引里存的是前驱，考生结点就是前驱的 next，平均 O(1)，不遍历链表
  Student* find(const unsigned int number) {
    ListNode<Student>* prev = findPrev(number);
    return prev == nullptr ? nullptr : &prev->next->data;
  }

  // 按考号删除，removed 返回被删除的考生信息。
  // 索引定位前驱 O(1) ＋ 按前驱摘链 O(1) ＋ 修正后继的前驱记录 O(1)
  bool remove(const unsigned int number, Student& removed) {
    ListNode<Student>* prev = findPrev(number);
    if (prev == nullptr) return false;
    ListNode<Student>* node = prev->next;
    removed = node->data;
    ListNode<Student>* next = node->next;  // 摘链前先记住后继
    index_.erase(number);                  // 删掉被删考生的考号
    students_.eraseAfter(prev);            // 改前驱的 next + 释放结点
    if (next != nullptr) {                 // 后继的前驱由 node 变成 prev
      const unsigned int nextNumber = next->data.getNumber();
      index_.erase(nextNumber);
      index_.insert(nextNumber, prev);
    }
    return true;
  }

  // 输出表头和全部考生信息，返回人数
  unsigned int printAll() const {
    std::cout << '\n';
    Student::printHeader();
    unsigned int count = 0;
    students_.forEach([&count](const Student& student) {
      student.print();
      count++;
    });
    if (count == 0) std::cout << "（当前没有考生信息）" << '\n';
    return count;
  }

  // 统计：总人数、性别分布、报考类别分布、平均年龄
  // 报考类别用哈希表分组计数，整体 O(n + k)（k 为报考类别数）
  void printStatistics() const {
    const unsigned int total = students_.size();
    std::cout << '\n' << "==================== 统计结果 ====================" << '\n';
    if (total == 0) {
      std::cout << "当前没有考生信息，无法统计。" << '\n';
      return;
    }

    unsigned int male = 0, female = 0, other = 0;
    unsigned long long ageSum = 0;
    HashMap<TypeKey, unsigned int> typeCounts;  // 类别 → 人数，查找/插入平均 O(1)
    List<TypeKey> typeOrder;  // 记住类别第一次出现的顺序，输出更直观

    students_.forEach([&](const Student& student) {
      if (strcmp(student.getGender(), "男") == 0)
        male++;
      else if (strcmp(student.getGender(), "女") == 0)
        female++;
      else
        other++;
      ageSum += student.getAge();

      TypeKey key = TypeKey();  // 先清零：哈希值是按整个键逐字节算的
      strcpy(key.type, student.getType());
      unsigned int* found = typeCounts.find(key);
      if (found != nullptr) {
        (*found)++;
      } else {
        typeCounts.insert(key, 1);
        typeOrder.push_back(key);
      }
    });

    std::cout << "考生总人数：" << total << " 人" << '\n';
    std::cout << "性别分布：男 " << male << " 人（";
    printPercent(male, total);
    std::cout << "），女 " << female << " 人（";
    printPercent(female, total);
    std::cout << "），其它 " << other << " 人（";
    printPercent(other, total);
    std::cout << "）" << '\n';

    std::cout << "报考类别分布（共 " << typeOrder.size() << " 类）：" << '\n';
    typeOrder.forEach([&](const TypeKey& key) {
      const unsigned int* count = typeCounts.find(key);  // 哈希查找，平均 O(1)
      if (count == nullptr) return;                      // 正常不会发生
      std::cout << "    " << key.type << "：" << *count << " 人（";
      printPercent(*count, total);
      std::cout << "）" << '\n';
    });

    const unsigned int ageAvg10 =
        (unsigned int)((ageSum * 10 + total / 2) / total);  // 四舍五入到 1 位小数
    std::cout << "平均年龄：" << ageAvg10 / 10 << '.' << ageAvg10 % 10 << " 岁"
              << '\n';
  }

 private:
  // 统计用的类别键：把报考类别字符串包一层，作为哈希表的键
  struct TypeKey {
    char type[kTypeLen + 1];
    bool operator==(const TypeKey& other) const {
      return strcmp(type, other.type) == 0;
    }
  };

  // 从哈希索引里取“考号对应结点的前驱”：平均 O(1)，找不到返回 nullptr。
  // 单链表没有前驱指针，所以查找、删除、改考号都要靠它先拿到前驱
  ListNode<Student>* findPrev(const unsigned int number) {
    ListNode<Student>** found = index_.find(number);  // V* find(key)
    return found == nullptr ? nullptr : *found;
  }

  // 读一条考号不与现有考生重复的考生信息
  Student readNewStudent(const char* prompt) {
    while (true) {
      const Student student = Input::readStudent(prompt);
      if (!exists(student.getNumber())) return student;
      std::cout << "输入不合法，考号 " << student.getNumber() << " 已存在，请重新输入！"
                << '\n';
    }
  }

  // 打印百分比，保留 1 位小数（整数运算完成四舍五入，不引入浮点格式控制）
  static void printPercent(const unsigned int part, const unsigned int total) {
    const unsigned int per10 =
        total == 0 ? 0 : (part * 1000 + total / 2) / total;
    std::cout << per10 / 10 << '.' << per10 % 10 << '%';
  }

  // 人数已达上限时的统一提示（插入与批量录入共用）
  static void printFullMessage() {
    std::cout << "插入失败，考生人数已达上限（" << kMaxStudentCount
              << " 人），请先删除考生后再插入!" << '\n';
  }

  List<Student> students_;
  // 考号 → 该考生结点的前驱：查找取前驱的 next，删除改前驱的 next，
  // 判重看键在不在，改考号只换键；四件事都是平均 O(1)
  HashMap<unsigned int, ListNode<Student>*> index_;
};
