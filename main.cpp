#include<stdio.h>
#include"main.h"
#include "Daemon.h"
#include "CdnRefreshInterface.h"
#include"Inotify.h"
#include"Initialize.h"
#include"QueueFilter.h"
#include"QueueRetry.h"
#include"FileSynchronize.h"
#include"SocketInterface.h"
#include"CommandInterface.h"
//#include"HttpInterface.h"
#include"Interface.h"
#include"main.h"

#define DEBUG 0
int IN_SYNC = 0; // inotify monitor param:in_create in_close_write

void DebugStart(int level);

void FileSynchronizeStart(boost::shared_ptr<Initialize> init);

void ModuleInterfaceStart(string config_file_name, Interface* module);

ptrQFilter qf(new QueueFilter); //全局智能指针对象事件过滤队列

ptrQRetry qr(new QueueRetry); //全局智能指针对象失败事件队列

int debug_level = 0; //debug等级

int main(int argc, char *argv[])
{
    DebugStart(DEBUG); //debug 开关 0关，3打印信息关开 2非线程 1主函数
    boost::shared_ptr<Initialize> init(new Initialize(argc, argv)); //初始化
    //开启守护线程
    if (init->exec_flag & OPEN_DAEMON)
    {
        Daemon_Start();
    }

    //开启同步模块
    if (init->module == "sersync")
    {
        FileSynchronizeStart(init);
    }
    //不执行同步单独开启各插件模块
    if (init->module == "other")
    {
        if (init->exec_flag & REFRESHCDN_MODULE)
        {
            cout << "启动刷新CDN模块" << endl;
            CdnRefreshInterface refresh;
            ModuleInterfaceStart(init->config_file_name, &refresh);

        } else if (init->exec_flag & SOCKET_MODULE)
        {
            cout << "启动SOCKET接口" << endl;
            SocketInterface socket;
            ModuleInterfaceStart(init->config_file_name, &socket);
        } else if (init->exec_flag & COMMAND_MODULE)
        {
            cout<< "start command interface"<<endl;
            CommandInterface command;
            ModuleInterfaceStart(init->config_file_name,&command);
        }
        //else if (init->exec_flag & HTTP_MODULE)
        //{
        //	cout << "启动HTTP接口" << endl;
        //	HttpInterface http;
        //	ModuleInterfaceStart( init->config_file_name, &http );
        //}

    }
    cout << "模块名称错误" << endl;
    exit(1);

}//end main


//=======================================================================
//函数名：  FileSynchronizeStart
//作者：    zy(zhoukunta@qq.com)
//日期：    2009-01-24
//功能：    启动各插件
//输入参数：config_file_name(string)配置文件名
//         module(Interface* )多态调用，各模块的共同基类指针
//返回值：  void
//修改记录：
//=======================================================================
void ModuleInterfaceStart(string config_file_name, Interface* module)
{
    module->XmlParse(config_file_name);
    Inotify inotify(module->m_watch);
    while (1)
    {
        inotify.GetEvents(qf);
        while (!qf->empty())
        {
            Event e = qf->pop();
            module->Execute(e);
        }
        if (debug_level & MAIN_INFO)
        {
            qf->printdeque();
        }
    }
}

//=======================================================================
//函数名：  FileSynchronizeStart
//作者：    zy(zhoukunta@qq.com)
//日期：    2009-01
//功能：    启动文件同步功能
//输入参数：init(boost::shared_ptr<Initialize>) 指向Initialize 对象的智能指针
//返回值：  void
//修改记录：
//=======================================================================

void FileSynchronizeStart(boost::shared_ptr<Initialize> init)
{

    boost::shared_ptr<FileSynchronize> syn(new FileSynchronize(init, qf, qr));
    cout << "run the sersync: " << endl;
    cout << "watch path is: " << syn->watch << endl;
    boost::shared_ptr<Inotify> inotify(new Inotify(syn->watch));
    while (1)
    {
        inotify->GetEvents(qf);
        syn->ThreadAwaken();
        if (debug_level & MAIN_INFO)
        {
            qf->printdeque();
            qr->printdeque();
        }
    }

}


//=======================================================================
//函数名：  DebugStart
//作者：    zy(zhoukunta@qq.com)
//日期：    2009-01
//功能：    开启debug，并按参数开启不同等级的打印信息
//输入参数：level(int) 取值范围(1,2,3) 3是全部开启，2是主函数与子类，1是主函数
//返回值：  无
//修改记录：
//=======================================================================
void DebugStart(int level)
{
    switch (level)
    {
        case 3:
            debug_level = debug_level | THREAD_DEBUG;
        case 2:
            debug_level = debug_level | SUB_CLASS;
        case 1:
            debug_level = debug_level | MAIN_INFO;
            break;
        default:
            debug_level = 0;
    }
}
