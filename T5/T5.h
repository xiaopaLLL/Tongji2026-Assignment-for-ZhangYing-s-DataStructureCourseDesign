// ============================================================================
//  题目五  关键字检索系统
//  文件结构：常量 → 单链表 List<T> → 控制台输入 Input → 关键字检索
//  KeywordSearcher 数据结构：带头结点的单链表（按行存放正文）＋
//  文本文件（保存正文并检索） 复杂度：存正文 O(n)；写文件 O(n)；检索用 KMP
//  逐行扫描，建失配表 O(m)、扫描 O(n + m)（m 为关键字长度）
//  说明：类与函数都定义在本文件里（成员函数直接在类里实现），T5.cpp里只有主函数；
//  链表是自己写的，没有使用任何 STL 容器；建立/读取文本文件用的是 C 标准库
//  的文件函数（cstdio），不是 C++ 的流，也不是 STL
// ============================================================================

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

// ============================== 常量 ==============================
const unsigned int kFileNameLen = 60;          // 文件名最长 60 字节
const unsigned int kKeywordLen = 100;          // 关键字最长 100 字节
const unsigned int kBodyLineLen = 512;         // 正文一行最长 512 字节
const unsigned int kMaxBodyLineCount = 10000;  // 正文最多 10000 行

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

  // 读一个不含空白的字段；out 的容量至少 maxLen + 1
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

  // 读一整行正文（允许空格、允许空行）：读成功返回 true，太长返回 false
  static bool readText(const char* prompt, char* out, const unsigned int cap) {
    std::cout << prompt;
    return readLine(out, cap);
  }

  // 连续录入正文时用：不打印提示，返回 false 表示输入结束（Ctrl+Z / 管道结束）
  static bool readBodyLine(char* out, const unsigned int cap) {
    while (true) {
      std::cin.getline(out, cap + 1);
      if (!std::cin.fail()) return true;
      if (std::cin.eof()) {
        std::cin.clear();
        return false;
      }
      std::cout << "输入过长，请重新输入！" << '\n';
      std::cin.clear();
      char c;
      while (std::cin.get(c) && c != '\n') {
      }
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

// ============================== 正文的一行 ==============================
// 固定长度的字符数组，可以直接按值放进单链表里（与题目一的链表完全一样）
struct TextLine {
  char text[kBodyLineLen + 1];
};

// ============================== 关键字检索系统 ==============================
// 先建立文本文件（文件名由用户输入，正文按行录入），再输入一个不含空格的
// 关键字，显示源文件内容并统计关键字在文件中出现的次数。
class KeywordSearcher {
 public:
  KeywordSearcher() {
    fileName_[0] = '\0';
    keyword_[0] = '\0';
    lineCount_ = 0;
  }

  // ---- 每个操作一个成员函数，主函数里直接 searcher.xxx() 调用 ----

  // 打印系统标题
  void printTitle() const { std::cout << "关键字检索系统" << '\n'; }

  // 读文件名（不检查文件是否存在，建立文件时再检查）
  void readFileName() {
    Input::readWord("请输入文件名：", "文件名", kFileNameLen, fileName_);
  }

  // 读要检索的关键字（不含空格、不超过 kKeywordLen）
  void readKeyword() {
    Input::readWord("请输入要检索的关键字： ", "关键字", kKeywordLen, keyword_);
  }

  // 正文按行录入：空行或输入结束（Ctrl+Z）都表示这一遍录完了
  void readBody() {
    while (true) {
      std::cout << "请输入一段英文：" << '\n';
      char line[kBodyLineLen + 1];
      lines_.clear();
      lineCount_ = 0;
      bool ended = false;  // 是否读到了输入结束
      while (lineCount_ < kMaxBodyLineCount) {
        if (!Input::readBodyLine(line, kBodyLineLen)) {
          ended = true;
          break;
        }
        if (line[0] == '\0') break;  // 空行表示正文结束
        TextLine item = TextLine();  // 先清零，再按行存进链表
        strcpy(item.text, line);
        lines_.push_back(item);
        lineCount_++;
      }
      if (lineCount_ > 0) return;
      std::cout << "输入不合法，正文不能为空，请重新输入！" << '\n';
      if (ended) return;  // 已经读不到新内容了，不再反复要求输入
    }
  }

  // 把正文写进文本文件；文件名非法（打不开）时提示并重新输入文件名
  void saveFile() {
    while (true) {
      FILE* file = fopen(fileName_, "w");
      if (file == nullptr) {
        std::cout << "输入不合法，无法创建文件" << fileName_
                  << "，请重新输入文件名！" << '\n';
        readFileName();
        continue;
      }
      lines_.forEach([file](const TextLine& item) {
        fputs(item.text, file);
        fputc('\n', file);
      });
      fclose(file);
      std::cout << "本段文本已保存在文本文件\"" << fileName_ << "\"中。"
                << '\n';
      return;
    }
  }

  // 显示源文件的内容
  void printFile() const {
    std::cout << "显示源文件\" " << fileName_ << " \"：" << '\n';
    List<TextLine> fileLines;
    if (!loadFile(fileLines)) {
      std::cout << "文件打开失败！" << '\n';
      return;
    }
    fileLines.forEach(
        [](const TextLine& item) { std::cout << item.text << '\n'; });
  }

  // 统计关键字在源文件中出现的次数：逐行用 KMP 扫描，匹配成功后按失配表回退，
  // 因此重叠出现的同一个关键字会分别计数（区分大小写）
  void printCount() const {
    List<TextLine> fileLines;
    if (!loadFile(fileLines)) {
      std::cout << "文件打开失败！" << '\n';
      return;
    }
    unsigned int count = 0;
    unsigned int
        failureTable[kKeywordLen + 1];  // 只在本函数里用，不作为成员保存
    buildFailureTable(failureTable);
    fileLines.forEach([&count, this, &failureTable](const TextLine& item) {
      count += kmpSearch(item.text, failureTable);
    });
    std::cout << "在源文件中共检索到： " << count << " 个关键字\" " << keyword_
              << " \"" << '\n';
  }

 private:
  // 把文本文件按行读进链表（行尾的换行符去掉）；文件打不开返回 false。
  // 这里只用 C 标准库的文件函数和 strcpy，fopen 与 fclose 一定成对出现。
  bool loadFile(List<TextLine>& fileLines) const {
    FILE* file = fopen(fileName_, "r");
    if (file == nullptr) return false;
    // 多留 2 字节：512 字节的行加上行尾的 \r\n 一共 514 字节，要一次读完
    char line[kBodyLineLen + 3];
    unsigned int lineCount = 0;
    while (lineCount < kMaxBodyLineCount &&
           fgets(line, sizeof(line), file) != nullptr) {
      unsigned int length = (unsigned int)strlen(line);
      while (length > 0 &&
             (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[length - 1] = '\0';
        length--;
      }
      // 文件被外部改过、单行超长时按上限截断，保证下面的 strcpy 不会溢出
      if (length > kBodyLineLen) line[kBodyLineLen] = '\0';
      TextLine item = TextLine();  // 先清零，再按行存进链表
      strcpy(item.text, line);
      fileLines.push_back(item);
      lineCount++;
    }
    fclose(file);
    return true;
  }

  // KMP 字符串模式匹配，返回 source 中 keyword_ 出现的次数。
  // failureTable 必须是 keyword_ 的失配表（由 buildFailureTable 建好）。
  // 匹配成功后按失配表回退，所以重叠出现的同一个关键字会分别计数。
  unsigned int kmpSearch(const char* source,
                         const unsigned int* failureTable) const {
    if (keyword_[0] == '\0') return 0;  // 空关键字：不加判断会越界读失配表
    unsigned int count = 0;
    unsigned int matchedLength = 0;
    for (unsigned int currentPos = 0; source[currentPos] != '\0';
         currentPos++) {
      while (matchedLength > 0 && source[currentPos] != keyword_[matchedLength])
        matchedLength = failureTable[matchedLength - 1];
      if (source[currentPos] == keyword_[matchedLength]) matchedLength++;
      if (keyword_[matchedLength] == '\0') {  // 匹配成功：计数后接着找
        count++;
        matchedLength = failureTable[matchedLength - 1];
      }
    }
    return count;
  }

  // 构建 KMP 失配表（用成员 keyword_）
  void buildFailureTable(unsigned int* failureTable) const {
    if (keyword_[0] == '\0') return;
    failureTable[0] = 0;
    unsigned int matchedLength = 0;
    for (unsigned int currentPos = 1; keyword_[currentPos] != '\0';
         currentPos++) {
      while (matchedLength > 0 &&
             keyword_[currentPos] != keyword_[matchedLength])
        matchedLength = failureTable[matchedLength - 1];
      if (keyword_[currentPos] == keyword_[matchedLength]) matchedLength++;
      failureTable[currentPos] = matchedLength;
    }
  }

  List<TextLine> lines_;             // 正文：与题目一相同的带头结点单链表
  char fileName_[kFileNameLen + 1];  // 文本文件名
  char keyword_[kKeywordLen + 1];    // 要检索的关键字

  unsigned int lineCount_;  // 正文行数
};
