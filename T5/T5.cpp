// ============================================================================
//  题目五  关键字检索系统 —— 主函数
// ============================================================================
#include "T5.h"

// ============================== 主函数 ==============================
int main() {
  KeywordSearcher searcher;

  searcher.printTitle();      // 输出系统标题
  searcher.readFileName();    // 读入文件名
  searcher.readBody();        // 按行录入正文（空行或 Ctrl+Z 结束）
  searcher.saveFile();        // 把正文保存成文本文件
  searcher.readKeyword();     // 读入要检索的关键字
  searcher.printFile();       // 显示源文件内容
  searcher.printCount();      // 统计关键字出现次数

  system("pause");

  return 0;
}
