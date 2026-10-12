// ============================================================================
//  题目六  家谱管理系统 —— 主函数
// ============================================================================
#include "T6.h"

// ============================== 主函数 ==============================
int main() {
  FamilyTree family;

  family.printMenu();    // 打印菜单
  family.buildFamily();  // 建立家谱（录入祖先）

  // 菜单循环
  while (true) {
    std::cout << '\n' << "请选择要执行的操作：";
    char operation = Input::readLetter("", "ABCDE");
    if (operation == 'A') family.completeFamily();  // A：完善家谱
    if (operation == 'B') family.addMember();       // B：添加成员
    if (operation == 'C') family.dissolveFamily();  // C：解散家庭
    if (operation == 'D') family.renameMember();    // D：重命名成员
    if (operation == 'E') break;                    // E：退出程序
  }

  system("pause");

  return 0;
}
