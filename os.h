#ifndef OS_H
#define OS_H

#include <QWidget>
#include <list>
#include <qdebug.h>
#include <QTimer>
struct pcb
{
    // 0号进程被预留，作为操作系统进程
    unsigned pid;
    unsigned needtime;
    int prio;
    unsigned size;
    unsigned base = 0;
    int state = 0;
    pcb *next = nullptr;
    bool independent = true;
    unsigned preProc,nextProc;

    // 构造函数
    pcb(unsigned id, unsigned t,int p, unsigned sz,unsigned prep, unsigned nextp):
    pid(id),needtime(t),prio(p), size(sz), preProc(prep), nextProc(nextp)
    {
        if (prep || nextp) independent = false;
    }
};


struct freeItem
{
    unsigned base;
    unsigned size;
    bool state = true;
    freeItem(unsigned b, unsigned s):base(b),size(s){}
};


namespace Ui {
class OS;
}

class OS : public QWidget
{
    Q_OBJECT

public:
    explicit OS(QWidget *parent = 0);
    ~OS();

private:
    Ui::OS *ui;
    QTimer *timer;
    int timercounter = 0;
private:
    //  内存基址之上的内存被操作系统占用
    int memoryBase = 16;
    int avaliableMemorySize = 64;
    std::list<freeItem> freeTable;
    // cpu指针 执行将要运行的cpu
    int cpuPtr = 0;
    // cpu数目，对应运行状态，可以增加，但要注意添加运行状态的名称
    static const int cpuCount = 2;
    // 状态对应队列的数目，对应包括运行状态和非运行状态（5个）
    static const int queueCount = cpuCount+6;
    // 单独设置后备队列尾指针是为了实现先进先出的作业调度算法，见exec()
    pcb *backingqueue_rail = nullptr;
    pcb *queue[queueCount];
    enum status{backing,ready,ready_suspended,blocked,blocked_suspended,finished,running0};
    // 状态对应的名称
    char statename[20][15] = {"后备", "就绪","静止就绪","阻塞","静止阻塞","完成","运行1","运行2"};

    bool create(pcb *proc)
    {
        if (!( running0 <= proc->state && proc->state < running0+cpuCount) && proc->state != blocked )
        {
            allocate(proc);
            if (proc->base == 0) return false;
        }

        // if (proc->state == running0+cpuCount) running = nullptr;
         if (proc->state == backing) queue[backing] = queue[backing]->next;
        auto p = queue[ready],q = queue[ready];
        for (; p!=nullptr && p->prio >= proc->prio; q = p, p = p->next);
        // if (p == queue[ready])
        // 	queue[ready] = proc;
        // else
        // 	q->next = proc;
        (p==queue[ready]?queue[ready]:q->next) = proc;
        proc->next = p;
        proc->state = ready;

        return true;
    }

    bool sched()
    {
        queue[running0+cpuPtr] = queue[ready];
        queue[running0+cpuPtr]->state = cpuPtr+running0;
        queue[ready] = queue[ready]->next;
        queue[running0+cpuPtr]->next = nullptr;
        pcb *b = getProcessByPid(queue[running0+cpuPtr]->preProc);
        if ((b==nullptr && queue[running0+cpuPtr]->preProc!=0) || (b!=nullptr && b->state != finished))
        {
            block();
            return false;
        }
        return true;
    }

    void jobSched()
    {
        // 尽最大能力给cpu分配任务
        while (queue[backing]!=nullptr && create(queue[backing]));
        if (queue[ready] == nullptr)
        {
            int nullcpucount = 0;
            while (nullcpucount != cpuCount && !queue[nullcpucount+running0]) ++nullcpucount;
            if(nullcpucount == cpuCount) timer->stop();
        }
    }

    void holt()
    {
        queue[running0+cpuPtr]->next = queue[finished];
        queue[finished] = queue[running0+cpuPtr];
        queue[finished]->state = finished;
        free(queue[finished]);
        queue[running0+cpuPtr] = nullptr;
        pcb *b = getProcessByPid(queue[finished]->nextProc);
        if (b!=nullptr && b->state == blocked)
            wakeup(b);
    }

    void block()
    {
        queue[running0+cpuPtr]->next = queue[blocked];
        queue[blocked] = queue[running0+cpuPtr];
        queue[blocked]->state = blocked;
        queue[running0+cpuPtr] = nullptr;
    }

    void wakeup(pcb *proc)
    {
        auto p = queue[blocked],q = queue[blocked];
        for (; p!=nullptr && p != proc;q = p,p=p->next);
        (q == p? queue[blocked]:q->next) = p->next;
        create(p);
    }

    void allocate(pcb *proc)
    {
        auto p = find_if(freeTable.begin(),
            freeTable.end(),
            [&](freeItem&item){
                return item.state == true && item.size >= proc->size ;
            });
        if (p == freeTable.end()) return;

        proc->base = p->base;
        // qDebug() << proc->pid << ends;
        // qDebug() << proc->base;
        // qDebug() << "=================" << statename[proc->state] <<  endl;
        auto remainSize = p->size - proc->size;
        p->size = proc->size;
        p->state = false;
        if (remainSize != 0)
            freeTable.emplace(++p,p->base+proc->size,remainSize);
    }

    void free(pcb *proc)
    {
        auto p = find_if(freeTable.begin(),
            freeTable.end(),
            [&](freeItem&item){
                return item.state == false && item.base == proc->base ;
            });
        auto q = p--;
        if (q!=freeTable.begin() && p->state==true)
        {
            q->base = p->base;
            q->size += p->size;
            freeTable.erase(p);
        }
        p = q++;
        if (q!=freeTable.end() && q->state==true)
        {
            p->size += q->size;
            freeTable.erase(q);
        }
        p->state = true;
        proc->base = 0;
    }

    pcb* getProcessByPid(unsigned id)
    {
        // 遍历所有队列（两个cpu看作一个元素的队列），遍历队列中的所有元素
        for (auto q:queue)
            for (pcb *p=q; p; p=p->next)
                if(p->pid == id) return p;
        return nullptr;
    }


public:
    void exec(unsigned id, int t,int p, unsigned sz,unsigned pre=0, unsigned next=0)
    {
        // 0号进程被预留，作为操作系统进程
        if (id == 0 || sz <= 0 || t<=0) return;
        // pid冲突则在pid后面添加0
        unsigned suffix = 1;
        while(getProcessByPid(id*suffix)) suffix*=10;
        auto newproc = new pcb(id*suffix,t,p,sz,pre,next);
        if (queue[backing] == nullptr) queue[backing] = backingqueue_rail = newproc;
        else
        {
            backingqueue_rail->next = newproc;
            backingqueue_rail = newproc;
        }
    }
    void suspend(unsigned id)
{
     for (int i = 0; i < cpuCount; ++i)
     {
        if (queue[running0+i] && id == queue[running0+i]->pid)
        {
            create(queue[running0+i]);queue[running0+i]=nullptr;
        }
     }
    auto state = getProcessByPid(id)->state;
    if (state!=ready && state !=blocked) return;
    auto p = queue[state],q = queue[state];
    for (; p!=nullptr && p->pid != id;q = p,p=p->next);
    if (p == nullptr) return;
    (q == p? queue[state]:q->next) = p->next;
    p->next = queue[state+1];
    queue[state+1] = p;
    queue[state+1]->state = state+1;
    free(queue[state+1]);
}


    void active(unsigned id)
    {
        auto state = getProcessByPid(id)->state;
        if (state!=ready_suspended && state!=blocked_suspended) return;
        auto p = queue[state],q = queue[state];
        for (; p!=nullptr && p->pid != id;q = p,p=p->next);
        if (p==nullptr) return;
        (q == p? queue[state]:q->next) = p->next;
        if (state==ready_suspended)
        {
            create(p);
        }else
        {
            allocate(p);
            if (p->base == 0) return;
            p->next = queue[blocked];
            queue[blocked] = p;
            p->state = blocked;
        }
    }

    void persecond()
    {
//        int nullcpucount = 0;
        for (int i = 0; i < cpuCount; ++i)
        {
            run();
            cpuPtr = (cpuPtr+1)%cpuCount;
//            if (!queue[running0+cpuPtr])++nullcpucount;
        }
//        if (nullcpucount == cpuCount) timer->stop();
    }
    void run();

    void printFT()
    {
        qDebug() << "\t空闲分区表:" << endl;
        qDebug() << "________________________\n";
        qDebug() << "始址\t|" << "大小\t|" << "状态\t|" << endl;
        for (auto item:freeTable)
            qDebug() << item.base << "\t|" << item.size << "\t|" << (item.state?"空闲":"已分配") << "\t|" << endl;
        qDebug() << "________________________|\n\n" ;
    }

    void printProcsss(pcb *p)
    {
        qDebug() << p->pid << "\t|" <<
                p->needtime << "\t|" <<
                p->prio << "\t|" <<
                p->base << "\t|" <<
                p->size << "\t|" <<
                statename[p->state] << "\t|" <<
                (p->independent?"独立":"同步")<< "\t|" <<
                p->preProc << "\t|" <<
                p->nextProc << "\t|" << endl;
    }

    void print()
    {
        // 对所有队列，打印任一队列中所有进程的信息
        qDebug() << "\t进程信息:" << endl;
        qDebug() << "________________________________________________________________________\n";
        qDebug() << "标识\t|" << "时间\t|" << "优先数\t|"
            << "始址\t|" << "大小\t|"<< "状态\t|" << "属性\t|" << "前驱\t|" << "后继\t|" << endl;
        for (auto q:queue)
            for (pcb *p=q; p; p=p->next)
                printProcsss(p);
        qDebug() << "________________________________________________________________________|\n\n";
    }
    void showpcb()
    {
        for (int i = 0; i < cpuCount; ++i)
            if(queue[i+running0]) showproc(queue[i+running0]);
        for (int i = backing; i < running0; ++i)
        {
            for (pcb *p=queue[i]; p; p=p->next)
                showproc(p);
        }
    }
    void showFT();
    void showproc(pcb *proc);

private slots:
    void on_contihButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_speedSlider_valueChanged(int value);
    void on_addButton_clicked();
    void on_suspendButton_clicked();
    void on_activeButton_clicked();
};

#endif // OS_H
