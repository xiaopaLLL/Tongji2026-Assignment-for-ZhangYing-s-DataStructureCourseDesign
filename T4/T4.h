// ============================================================================
//  题目四  N皇后问题
//  文件结构：常量 → 控制台输入 Input → N皇后游戏 QueensGame
//  数据结构：一维数组 columnOfRow_（第 i 行皇后所在的列），用递归回溯代替显式栈
//  复杂度：每行放一个皇后，冲突检查 O(N)，总时间 O(N!)，空间 O(N)
//  说明：类与函数都定义在本文件里（成员函数直接在类里实现），T4.cpp 里只有主函数；
//        没有使用任何 STL 容器
// ============================================================================

#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>

// ============================== 常量 ==============================
const unsigned int kMinQueenCount = 1;    // 皇后个数下限
const unsigned int kMaxQueenCount = 10;   // 皇后个数上限（摆法太多，输出会很长的）

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

// ============================== N皇后游戏 ==============================
// 第 i 个皇后固定放在第 i 行上，这样只需考虑“不同列、不同斜线”。
// 用回溯法逐行摆放：某行的 N 列都冲突就退回上一行换列。
class QueensGame {
 public:
  QueensGame() {
    count_ = 0;
    solutionCount_ = 0;
    for (unsigned int row = 0; row < kMaxQueenCount; row++) columnOfRow_[row] = 0;
  }

  // 打印题目说明
  void printRule() const {
    std::cout << "现有N×N的棋盘，放入N个皇后，要求所有皇后不在同一行、列和同一斜线上!"
              << '\n';
    std::cout << '\n';
  }

  // 读入皇后个数
  void readCount() {
    count_ = Input::readNumber("请输入皇后的个数：", kMinQueenCount, kMaxQueenCount);
  }

  // 求出全部摆法并逐个输出
  void solve() {
    solutionCount_ = 0;
    std::cout << '\n' << "皇后摆法：" << '\n' << '\n';
    place(0);
  }

  // 输出解法总数
  void printSolutionCount() const {
    std::cout << "共有" << solutionCount_ << "种解法!" << '\n';
  }

 private:
  // 在第 row 行摆皇后：这一行的每一列都试一遍，能摆就摆上再摆下一行
  void place(const unsigned int row) {
    if (row == count_) {  // N 个皇后都摆好了，得到一种摆法
      solutionCount_++;
      printBoard();
      std::cout << '\n';
      return;
    }
    for (unsigned int col = 0; col < count_; col++) {
      if (isSafe(row, col)) {
        columnOfRow_[row] = col;
        place(row + 1);
      }
    }
  }

  // 检查第 row 行第 col 列能不能摆：只需与前 row 个皇后比较
  bool isSafe(const unsigned int row, const unsigned int col) const {
    for (unsigned int r = 0; r < row; r++) {
      if (columnOfRow_[r] == col) return false;  // 同一列
      const unsigned int rowGap = row - r;
      const unsigned int colGap =
          columnOfRow_[r] > col ? columnOfRow_[r] - col : col - columnOfRow_[r];
      if (rowGap == colGap) return false;  // 同一斜线
    }
    return true;
  }

  // 打印一种摆法：X 表示皇后，0 表示空格，同一行的格子之间空一格
  void printBoard() const {
    for (unsigned int row = 0; row < count_; row++) {
      for (unsigned int col = 0; col < count_; col++) {
        if (col > 0) std::cout << ' ';
        std::cout << (columnOfRow_[row] == col ? 'X' : '0');
      }
      std::cout << '\n';
    }
  }

  unsigned int columnOfRow_[kMaxQueenCount];  // 第 i 行皇后所在的列
  unsigned int count_;                        // 皇后个数 N
  unsigned int solutionCount_;                // 已经找到的摆法数
};
