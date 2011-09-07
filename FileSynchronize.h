/* 
 * File:   ServersFileSysc.h
 * Author: root
 *
 * Created on 2009年12月21日, 下午1:29
 */
#include<string>
#include"Initialize.h"
#include"Interface.h"
#include"main.h"
#include <boost/shared_ptr.hpp>
#ifndef _SERVERSFILESYSC_H
#define	_SERVERSFILESYSC_H

class FileSynchronize {
public:
    static std::string m_plugin;
    ptrInitialize m_init;
    ptrQFilter m_qf;
    ptrQRetry m_qr;
    static std::string watch;
    static std::vector<ptrRmtServer> rmtServers;
    static int m_crontab;
    static int sleep_group; //用于统计当前处于sleep状态的线程数量
    static boost::condition_variable work_cond; //工作条件对象
    static boost::mutex worklock; //用于统计工作线程数量的锁定
    static std::string m_users;
    static std::string m_password;
    static std::string m_delete;
    static vector<string> cfilter;
    static int firstflag;
    static std::string ssh;
    static std::string port;
    static std::string file_name;
    static int time;
    static bool debug;
    static string timeout;
    static string params;
public:
    FileSynchronize(ptrInitialize init, ptrQFilter qf, ptrQRetry qr);
    static int XmlParse(ptrInitialize init); 
    void ThreadAwaken();
    static bool RsyncOnce();  
protected:
    static std::string StrEscaped(std::string);//escaped the '$' in the path
    static std::string AddExclude(); 
    static void ExecuteScript(std::string file_name); 
    static void* ServerSyncThread(ptrQFilter qf, ptrQRetry qr, ptrInterface itf);
    static void* RsyncThread(ptrQRetry qr, Event event, std::string ip, std::string module, std::string watch);
    static void* QueueRetryThread(ptrQRetry qr);
    static std::string DelPathCombine(std::string str, bool dir);


};

#endif	/* _SERVERSFILESYSC_H */

