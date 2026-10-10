// ============================================================================
//  题目一  考试报名系统 —— 主函数
// ============================================================================
#include "T1.h"

// ============================== 主函数 ==============================
int main() {
  StudentSystem sys;
  sys.build();  // 建立考生信息系统

  // 菜单循环
  while (true) {
    std::cout << '\n'
              << "请选择您要进行的操作（1为插入，2为删除，3为查找，4为修改，"
                 "5为统计，0为取消操作）"
              << '\n';
    unsigned int operation = Input::readNumber("请选择您要进行的操作：", 0, 5);
    if (operation == 0) {  // 选项0取消操作
      std::cout << "已退出考试报名系统!" << '\n';
      break;
    }
    if (operation == 1) sys.addStudent();       // 选项1插入
    if (operation == 2) sys.removeStudent();    // 选项2删除
    if (operation == 3) sys.findStudent();      // 选项3查找
    if (operation == 4) sys.modifyStudent();    // 选项4修改
    if (operation == 5) sys.printStatistics();  // 选项5统计
  }

  system("pause");

  return 0;
}
