// ============================================================================
//  题目三  勇闯迷宫游戏
//  文件结构：常量 → 顺序栈 Stack<T> → 控制台输入 Input → 迷宫 Maze
//  数据结构：二维字符数组（迷宫地图）＋ 二维标记数组（visited 剪枝）＋ 顺序栈（保存每个点的下标与试探方向）
//  复杂度：用 visited_ 剪枝后每个点最多进栈一次、四个方向各试探一次，
//          时间 O(行×列)，空间 O(行×列)（有通路、无通路都是这个上界）
//  说明：类与函数都定义在本文件里（成员函数直接在类里实现），T3.cpp 里只有主函数；
//        栈是自己写的，没有使用任何 STL 容器；
//        方向顺序取“右、下、左、上”；
//        整张地图全是障碍时入口、出口没有合法位置可放，build() 会判为非法输入
//        并要求重新输入整张地图，search() 里再兜底一次防止越界访问；
//        cells_ 上的 x 只是“当前栈里的通路”，回退时要撤销（失败后地图不留 x）；
//        visited_ 是“这个点已经进过栈”的持久标记，回退不撤销，
//        它保证同一个点不会被重复进栈，从而避免反复枚举所有通路
// ============================================================================

#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>

// ============================== 常量 ==============================
const unsigned int kMinRowCount = 2;      // 迷宫最少 2 行
const unsigned int kMaxRowCount = 20;     // 迷宫最多 20 行
const unsigned int kMinColCount = 2;      // 迷宫最少 2 列
const unsigned int kMaxColCount = 10;     // 迷宫最多 10 列（每列占 8 列宽，一屏一行放得下）
const unsigned int kRowLineLen = 200;     // 读一行迷宫地图最长 200 字节
const unsigned int kMaxPathLen = kMaxRowCount * kMaxColCount;  // 路径最长 200 个点
const char kWall = '#';                   // 障碍
const char kFree = '0';                   // 通路
const char kPath = 'x';                   // 已经走过的点（最终就是通路）

// ============================== 顺序栈 ==============================
// 顺序栈：容量固定，后进先出；data_[0] 是栈底，size_ 是栈内元素个数。
// 栈里只存下标、方向或结点指针这类小数据，容量不足时 push 返回 false。
template <typename T>
class Stack {
 public:
  explicit Stack(const unsigned int cap) {
    cap_ = cap < 1 ? 1 : cap;  // 至少留 1 个位置，容量算错时栈也还能用
    data_ = new T[cap_];
    size_ = 0;
  }

  Stack(const Stack&) = delete;  // 内部有裸指针，禁止拷贝
  Stack& operator=(const Stack&) = delete;
  ~Stack() { delete[] data_; }

  bool empty() const { return size_ == 0; }
  unsigned int size() const { return size_; }

  bool push(const T& value) {
    if (size_ == cap_) return false;  // 栈满
    data_[size_] = value;
    size_++;
    return true;
  }

  // 出栈：把栈顶元素取到 out 里；栈空返回 false
  bool pop(T& out) {
    if (size_ == 0) return false;
    size_--;
    out = data_[size_];
    return true;
  }

  // 栈顶元素（可读可写）；由调用者保证栈非空
  T& top() { return data_[size_ - 1]; }
  const T& top() const { return data_[size_ - 1]; }

  // 从栈底开始按下标访问（输出整条路径时用）；由调用者保证下标合法
  const T& at(const unsigned int index) const { return data_[index]; }

  void clear() { size_ = 0; }

 private:
  T* data_;
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

  // 读一行并按空白切成若干字段：返回字段个数（0 表示空行）；
  // 返回 -1 表示这一行过长（提示已打印），调用者应重新读一次
  static int readFields(const char* prompt, char* line, char* fields[],
                        const unsigned int maxFields, const unsigned int cap) {
    std::cout << prompt;
    if (!readLine(line, cap)) return -1;
    return splitFields(line, fields, maxFields);
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

// ============================== 迷宫 ==============================
// 回溯法求通路：从入口出发按“右、下、左、上”的顺序试探，
// 能走就前进并把（点，方向）压栈，四个方向都走不通就弹栈退回上一点。
// 栈里保存的正是“每个能达到的点的下标以及该点前进的方向”。
// 另外用一个 visited_ 记“这个点已经进过栈”，压栈前先看它：
// 进过栈的点不再进栈，所以弹栈回退后不会从别的邻点又绕回来，
// 搜索不会退化成把所有通路都枚举一遍（无通路时也能在 O(行×列) 内结束）。
struct MazeStep {
  int row;        // 所在行
  int col;        // 所在列
  int direction;  // 下一个要试探的方向：0 右、1 下、2 左、3 上
};

class Maze {
 public:
  Maze() {
    rowCount_ = 0;
    colCount_ = 0;
    startRow_ = 0;
    startCol_ = 0;
    endRow_ = 0;
    endCol_ = 0;
    pathLen_ = 0;
    found_ = false;
    for (unsigned int row = 0; row < kMaxRowCount; row++)
      for (unsigned int col = 0; col < kMaxColCount; col++) {
        cells_[row][col] = kFree;
        visited_[row][col] = false;
      }
  }

  // ---- 每个操作一个成员函数，主函数里直接 maze.xxx() 调用 ----

  // 读入地图大小、地图本身以及入口、出口
  // 整张地图全是障碍时判为非法输入，提示之后把整张地图重新读一遍。
  void build() {
    rowCount_ = Input::readNumber("请输入迷宫的行数：", kMinRowCount, kMaxRowCount);
    colCount_ = Input::readNumber("请输入迷宫的列数：", kMinColCount, kMaxColCount);
    while (true) {
      std::cout << "请输入迷宫地图（# 表示障碍，0 表示通路）：" << '\n';
      for (unsigned int row = 0; row < rowCount_; row++) readRow(row);
      if (!isAllWall()) break;  // 至少有一个通路格子，入口、出口才有地方可放
      std::cout << "输入不合法，迷宫不能全是障碍（至少要有一个 " << kFree
                << " 作为入口和出口），请重新输入整张地图！" << '\n';
    }

    printMap();
    readStart();
    readEnd();
  }

  // 回溯法搜索通路：找到返回 true，并把通路按从入口到出口的顺序抄进 path_
  // walkable(...) 只判“这个格子能不能站”（是通路 0），explorable(...) 再要求“还没进过栈”；
  // 用 explorable 压栈，同一个点就只会进栈一次，不会绕回来反复试探。
  bool search() {
    // 入口、出口越界或落在障碍上（例如整张地图全是障碍）时直接结束搜索，
    // 避免带着非法下标去访问 cells_；此时 found_ 保持 false，由 printPath 给出提示。
    if (!walkable((int)startRow_, (int)startCol_) ||
        !walkable((int)endRow_, (int)endCol_)) {
      found_ = false;
      pathLen_ = 0;
      return false;
    }

    clearVisited();
    Stack<MazeStep> stack(kMaxPathLen);  // 容量 = 格子总数，配合 visited_ 不会溢出
    MazeStep start;
    start.row = (int)startRow_;
    start.col = (int)startCol_;
    start.direction = 0;
    if (!stack.push(start)) return false;
    visited_[startRow_][startCol_] = true;  // 进过栈就不能再进
    cells_[startRow_][startCol_] = kPath;   // 走过的点做标记，避免来回绕圈

    while (!stack.empty()) {
      MazeStep& current = stack.top();
      if (current.row == (int)endRow_ && current.col == (int)endCol_) {
        pathLen_ = stack.size();
        for (unsigned int i = 0; i < pathLen_; i++) path_[i] = stack.at(i);
        found_ = true;
        return true;
      }
      if (current.direction >= 4) {  // 四个方向都试过：退回上一点
        cells_[current.row][current.col] = kFree;  // 只撤销显示用的 x，visited_ 保留
        MazeStep back;
        stack.pop(back);
        continue;
      }
      int rowStep = 0;
      int colStep = 0;
      directionDelta((unsigned int)current.direction, rowStep, colStep);
      current.direction++;
      const int nextRow = current.row + rowStep;
      const int nextCol = current.col + colStep;
      if (!explorable(nextRow, nextCol)) continue;
      MazeStep next;
      next.row = nextRow;
      next.col = nextCol;
      next.direction = 0;
      if (!stack.push(next)) return false;  // 栈满（正常不会发生）
      visited_[nextRow][nextCol] = true;
      cells_[nextRow][nextCol] = kPath;
    }
    found_ = false;
    return false;
  }

  // 输出迷宫地图：行号、列号都与题目示例一致，走过的点显示为 x
  void printMap() const {
    std::cout << "迷宫地图:" << '\n';
    std::cout << '\n';
    for (unsigned int col = 0; col < colCount_; col++) std::cout << '\t' << col << "列";
    std::cout << '\n';
    for (unsigned int row = 0; row < rowCount_; row++) {
      std::cout << row << "行";
      for (unsigned int col = 0; col < colCount_; col++)
        std::cout << '\t' << cells_[row][col];
      std::cout << '\n' << '\n';
    }
    std::cout << '\n';
  }

  // 输出通路：<行,列> ---> <行,列> ---> ... ---> <出口>
  void printPath() const {
    std::cout << "迷宫路径:" << '\n';
    std::cout << '\n';
    if (!found_) {
      std::cout << "迷宫中没有从入口到出口的通路！" << '\n';
    } else {
      for (unsigned int i = 0; i < pathLen_; i++) {
        if (i > 0) std::cout << " ---> ";
        std::cout << '<' << path_[i].row << ',' << path_[i].col << '>';
      }
      std::cout << '\n';
    }
    std::cout << '\n' << '\n';
  }

 private:
  // 读入口：先读行号再读列号，入口不能设在障碍上
  void readStart() {
    while (true) {
      startRow_ = Input::readNumber("请输入入口的行号：", 0, rowCount_ - 1);
      startCol_ = Input::readNumber("请输入入口的列号：", 0, colCount_ - 1);
      if (cells_[startRow_][startCol_] != kWall) return;
      std::cout << "输入不合法，入口不能设在障碍上，请重新输入！" << '\n';
    }
  }

  // 读出口：先读行号再读列号，出口不能设在障碍上
  void readEnd() {
    while (true) {
      endRow_ = Input::readNumber("请输入出口的行号：", 0, rowCount_ - 1);
      endCol_ = Input::readNumber("请输入出口的列号：", 0, colCount_ - 1);
      if (cells_[endRow_][endCol_] != kWall) return;
      std::cout << "输入不合法，出口不能设在障碍上，请重新输入！" << '\n';
    }
  }

  // 读一行迷宫地图：既允许“# 0 0 #”这样用空白分隔，也允许“#00#”连写
  void readRow(const unsigned int row) {
    char line[kRowLineLen + 1];
    char* fields[kMaxColCount + 2];
    while (true) {
      std::cout << "请输入第" << row << "行的迷宫（# 表示障碍，0 表示通路）：";
      const int count = Input::readFields("", line, fields, kMaxColCount + 1, kRowLineLen);
      if (count < 0) continue;  // 这一行太长，提示已经打印过
      if (!parseRow(fields, count, row)) {
        std::cout << "输入不合法，每行应有 " << colCount_ << " 个格子（只能是 "
                  << kWall << " 或 " << kFree << "），请重新输入！" << '\n';
        continue;
      }
      return;
    }
  }

  // 把一行切出来的字段写进地图：1 个字段按连写处理，多个字段按一格一个字符处理
  bool parseRow(char* fields[], const int count, const unsigned int row) {
    if (count == 1) {
      const char* text = fields[0];
      if (strlen(text) != colCount_) return false;
      for (unsigned int col = 0; col < colCount_; col++)
        if (!isCellChar(text[col])) return false;
      for (unsigned int col = 0; col < colCount_; col++) cells_[row][col] = text[col];
      return true;
    }
    if (count != (int)colCount_) return false;
    for (int i = 0; i < count; i++)
      if (strlen(fields[i]) != 1 || !isCellChar(fields[i][0])) return false;
    for (unsigned int col = 0; col < colCount_; col++) cells_[row][col] = fields[col][0];
    return true;
  }

  static bool isCellChar(const char cell) { return cell == kWall || cell == kFree; }

  // 地图是不是整张都是障碍（只要找到一个通路格子就不是）
  bool isAllWall() const {
    if (rowCount_ == 0 || colCount_ == 0) return false;
    for (unsigned int row = 0; row < rowCount_; row++)
      for (unsigned int col = 0; col < colCount_; col++)
        if (cells_[row][col] != kWall) return false;
    return true;
  }

  // 第 direction 个方向的行、列增量：0 右、1 下、2 左、3 上
  static void directionDelta(const unsigned int direction, int& rowStep, int& colStep) {
    if (direction == 0) {
      rowStep = 0;
      colStep = 1;
    } else if (direction == 1) {
      rowStep = 1;
      colStep = 0;
    } else if (direction == 2) {
      rowStep = 0;
      colStep = -1;
    } else {
      rowStep = -1;
      colStep = 0;
    }
  }

  // 越界或是障碍就不能走
  bool walkable(const int row, const int col) const {
    if (row < 0 || col < 0) return false;
    if ((unsigned int)row >= rowCount_ || (unsigned int)col >= colCount_) return false;
    return cells_[row][col] == kFree;
  }

  // 能走、而且还没有进过栈 —— 压栈前用它判断，保证一个点只进栈一次
  bool explorable(const int row, const int col) const {
    return walkable(row, col) && !visited_[row][col];
  }

  // 清空 visited_：每次开始搜索前调用，保证同一次搜索里标记有效
  void clearVisited() {
    for (unsigned int row = 0; row < kMaxRowCount; row++)
      for (unsigned int col = 0; col < kMaxColCount; col++) visited_[row][col] = false;
  }


  char cells_[kMaxRowCount][kMaxColCount];      // 迷宫地图：墙、通路、当前通路上的点
  bool visited_[kMaxRowCount][kMaxColCount];    // 这个点是否已经进过栈（回退不撤销）
  MazeStep path_[kMaxPathLen];                  // 最终通路（按从入口到出口的顺序）
  unsigned int rowCount_;
  unsigned int colCount_;
  unsigned int startRow_;
  unsigned int startCol_;
  unsigned int endRow_;
  unsigned int endCol_;
  unsigned int pathLen_;
  bool found_;
};
