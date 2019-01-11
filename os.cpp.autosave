#include "os.h"
#include "ui_os.h"
#define titem(i) (new QTableWidgetItem(i))
#define item(i) (new QTableWidgetItem(QString::asprintf("%d",i)))
#define num(i) (i->text().toInt())
OS::OS(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OS)
{

    ui->setupUi(this);
//    ui->pcbtableWidget->resizeColumnsToContents();
    ui->pcbtableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->freetableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 初始化空闲分区表
    freeTable.emplace(freeTable.begin(), memoryBase,avaliableMemorySize);
    // 初始化所有队列
    for (int i = 0; i < queueCount; ++i)
        queue[i] = nullptr;
    // 运行一些作业，放入后备队列
    exec(1,3,100,2,0,2);
    exec(2,3,100,4,1,3);
    exec(3,3,100,2,2,0);
    exec(4,4,4,4);
    exec(5,5,5,8);
    exec(6,6,6,2);
    exec(7,7,7,4);
    exec(8,8,8,4);
    exec(9,9,9,4);
    exec(10,10,10,4);
//    exec(1,3,100,2,0,2);
//    exec(2,3,100,4,1,3);
//    exec(3,3,100,2,2,0);
//    exec(4,4,4,4);
//    exec(5,5,5,8);
//    exec(6,6,6,2);
//    exec(7,7,7,4);
//    exec(8,8,8,4);
//    exec(9,9,9,4);
//    exec(10,10,10,4);

//    connect(timer,SIGNAL(timeout(),this,SLOT(on_contihButton_clicked());
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_contihButton_clicked()));
    timer->setInterval(1000);

    showpcb();
    showFT();
}

OS::~OS()
{
    delete ui;
}


void OS::run()
{
    // running 是当前的执行指针
    pcb *&running = queue[cpuPtr+running0];

    //运行前： 检查是否需要调度，检查是否需要阻塞
    // 为了提高效率，在就绪队列无进程或者被阻塞之后进行作业调度
    if (running == nullptr)
    {
        jobSched();
        while(queue[ready] && !sched());
        if (queue[cpuPtr+running0] == nullptr) return;
    }

    // 执行过程中：
    // 进程控制相关操作花费的时间应该远小于一个单位时间，这里我直接忽略了
    // 所以该执行一个单位时间必然导致所需时间和优先数的减小
    running->needtime--;
    running->prio--;

    // 执行后：检查是否终止进程并唤醒其后继进程， 检查是否换入优先级更高的进程
    if (running->needtime == 0)
    {
        holt();
        while(queue[ready] && !sched());
    }
    else if (queue[ready]!=nullptr && running->prio < queue[ready]->prio)
    {
        create(running);
        running = nullptr;
        while(queue[ready] && !sched());
    }
}

void OS::showFT()
{

    for(auto it : freeTable)
    {
        int row = ui->freetableWidget->rowCount();
        ui->freetableWidget->insertRow(row);
        int i = 0;
        ui->freetableWidget->setItem(row,i++,item(it.base));
        ui->freetableWidget->setItem(row,i++,item(it.size));
        ui->freetableWidget->setItem(row,i++,titem(it.state?"空闲":"已分配"));
    }


}

void OS::showproc(pcb *proc)
{
    int row = ui->pcbtableWidget->rowCount();
    ui->pcbtableWidget->insertRow(row);
    int i = 0;
    ui->pcbtableWidget->setItem(row,i++,item(proc->pid));
    ui->pcbtableWidget->setItem(row,i++,titem(statename[proc->state]));
    ui->pcbtableWidget->setItem(row,i++,item(proc->needtime));
    ui->pcbtableWidget->setItem(row,i++,item(proc->prio));
    ui->pcbtableWidget->setItem(row,i++,item(proc->base));
    ui->pcbtableWidget->setItem(row,i++,item(proc->size));
    ui->pcbtableWidget->setItem(row,i++,titem(proc->independent?"独立":"同步"));
    ui->pcbtableWidget->setItem(row,i++,item(proc->preProc));
    ui->pcbtableWidget->setItem(row,i++,item(proc->nextProc));
}

void OS::on_contihButton_clicked()
{
    ui->pcbtableWidget->setRowCount(0);
    ui->freetableWidget->setRowCount(0);
    persecond();
    showpcb();
    showFT();
    ui->lcdNumber->display(++timercounter);
}

void OS::on_startButton_clicked()
{
    timer->start();
}

void OS::on_stopButton_clicked()
{
    timer->stop();
    for (int i = 0; i < queueCount; ++i)
        qDebug() << statename[i] << queue[i];
}


void OS::on_speedSlider_valueChanged(int value)
{
    int ms = ui->speedSlider->maximum() + ui->speedSlider->minimum() - value;
    ms *= 100;
    timer->setInterval(ms);
}

void OS::on_addButton_clicked()
{
    ui->pcbtableWidget->setRowCount(0);
    ui->freetableWidget->setRowCount(0);
    exec(num(ui->pidEdit),num(ui->timeEdit),num(ui->prioEdit),num(ui->memEdit),num(ui->preprocEdit),num(ui->nextprocEdit));
    showpcb();
    showFT();
}

void OS::on_suspendButton_clicked()
{
    ui->pcbtableWidget->setRowCount(0);
    ui->freetableWidget->setRowCount(0);
    suspend(num(ui->pidEdit));
    showpcb();
    showFT();
}

void OS::on_activeButton_clicked()
{
    ui->pcbtableWidget->setRowCount(0);
    ui->freetableWidget->setRowCount(0);
    active(num(ui->pidEdit));
    showpcb();
    showFT();
}
