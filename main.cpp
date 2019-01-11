#include "os.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OS w;
    w.setWindowTitle("操作系统进程调度和内存分配回收模拟");
    w.show();

    return a.exec();
}
