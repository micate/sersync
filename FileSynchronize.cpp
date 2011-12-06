#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/inotify.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <fcntl.h>
#include "QueueFilter.h"
#include "Inotify.h"
#include "xmlParser.h"
#include "QueueRetry.h"
#include "FileSynchronize.h"
#include "Initialize.h"
#include "main.h"
#include "SocketInterface.h"
//#include "HttpInterface.h"
#include "CommandInterface.h"
#include "CdnRefreshInterface.h"

//#define SCRIPT_EXECUTE_INTERVAL 3600 //每隔1个小时执行一次rsync_fail_log.sh

extern int IN_SYNC;

int FileSynchronize::sleep_group = 0; //用于统计当前处于sleep状态的线程数量

boost::condition_variable FileSynchronize::work_cond; //工作条件对象

boost::mutex FileSynchronize::worklock; //用于统计工作线程数量的锁定

int FileSynchronize::m_crontab = -1; //默认不开起crontab进行整体推送

string FileSynchronize::m_users = "";

string FileSynchronize::m_delete = "";

string FileSynchronize::m_password = "";

bool FileSynchronize::debug = false;

std::string FileSynchronize::ssh = "";

std::string FileSynchronize::port = "";

std::string FileSynchronize::timeout = "";

std::string FileSynchronize::watch;

std::string FileSynchronize::m_plugin;

std::vector<ptrRmtServer> FileSynchronize::rmtServers;

std::vector<string> FileSynchronize::cfilter;

std::string FileSynchronize::file_name = "";

int FileSynchronize::time = 7200;

std::string FileSynchronize::params = "";

int FileSynchronize::XmlParse(boost::shared_ptr<Initialize> init) {

    if (!(IN_SYNC & IN_DELETE)) m_delete = "";
    XMLNode xMainNode = XMLNode::openFileHelper(init->config_file_name.c_str(), "head");
    XMLNode xNode = xMainNode.getChildNode("sersync");

    string temp = xNode.getChildNode("failLog").getAttribute("path");
    if (!temp.empty()) file_name = temp;

    temp = xNode.getChildNode("failLog").getAttribute("timeToExecute");
    if (!temp.empty()) time = atoi(temp.c_str()) * 60;

    //parse crontab tag
    temp = xNode.getChildNode("crontab").getAttribute("start");
    if (temp == "true") {
        cout << "Start the crontab " << "\t";
        m_crontab = atoi(xNode.getChildNode("crontab").getAttribute("schedule"));
        if (m_crontab > 0) {
            cout << "Every " << m_crontab << " minutes rsync all the files to the remote servers entirely" << endl;
        } else {
            m_crontab = 600;
        }

        temp = xNode.getChildNode("crontab").getChildNode("crontabfilter").getAttribute("start");
        if (temp == "true") {
            cout << "开启crontab过滤功能:\t" << "作整体同步的时候会对如下文件进行过滤" << endl;
            int num = xNode.getChildNode("crontab").getChildNode("crontabfilter").nChildNode("exclude");
            for (int i = 0; i < num; i++) {
                string t = xNode.getChildNode("crontab").getChildNode("crontabfilter").getChildNode(i).getAttribute("expression");
                cout << t << endl;
                cfilter.push_back(t);
            }
        }

        if ((!Initialize::filter.empty()) && cfilter.empty()) {
            cout << "************************WARNING***********************************" << endl;
            cout << "您设置了文件过滤器但没有设置crontab过滤器，所以crontab会将您已经过滤的" << endl;
            cout << "文件整体同步到目标主机，为了达到过滤效果，请根据您设置的文件过滤器设置" << endl;
            cout << "crontab过滤器,注意两者正则语法可能不同,请您参考默认配置文件内容。" << endl;
            cout << "如果您不设置crontab过滤器，crontab将会关闭" << endl;
            m_crontab = -1;
        }
    }

    //parse rsync label
    temp = xNode.getChildNode("rsync").getChildNode("auth").getAttribute("start");
    if (temp == "true") {
        cout << "use rsync password-file :" << endl;
        m_users = xNode.getChildNode("rsync").getChildNode("auth").getAttribute("users");
        m_password = xNode.getChildNode("rsync").getChildNode("auth").getAttribute("passwordfile");
        cout << "user is\t" << m_users << endl;
        cout << "passwordfile is \t" << m_password << endl;
        if (!m_password.empty()) m_password = " --password-file=" + m_password;
        if (!m_users.empty()) m_users += "@";
    }

    temp = xNode.getChildNode("rsync").getChildNode("ssh").getAttribute("start");
    if (temp == "true") ssh = " -e ssh ";


    temp = xNode.getChildNode("rsync").getChildNode("userDefinedPort").getAttribute("start");
    if (temp == "true") {
        string p = xNode.getChildNode("rsync").getChildNode("userDefinedPort").getAttribute("port");
        port = " --port=" + p + " ";
    }

    temp = xNode.getChildNode("rsync").getChildNode("timeout").getAttribute("start");
    if (temp == "true") {
        string p = xNode.getChildNode("rsync").getChildNode("timeout").getAttribute("time");
        timeout = " --timeout=" + p + " ";
    }

    temp = xNode.getChildNode("rsync").getChildNode("commonParams").getAttribute("params");
    if (!temp.empty()) params = temp;

    temp = xNode.getChildNode("plugin").getAttribute("start");
    if (temp == "true") {
        cout << "after each synchronize run the plugin " << "\t";
        m_plugin = xNode.getChildNode("plugin").getAttribute("name");
        cout << "plugin name is: " << m_plugin << endl;
    }

    XMLNode lNode = xNode.getChildNode("localpath", 0);
    int wtchnum = xNode.nChildNode("localpath"); //get the numbers of the localpath tag
    if (wtchnum == 0) {
        perror("ERROR,You must specify the watch localpath\n");
        exit(1);
    }
    watch = xNode.getChildNode("localpath").getAttribute("watch");
    watch = Initialize::SplitLastSlash(watch);

    /*open debug*/
    if (init->debug == 1) debug = 1;
    if (init->deleteFlag == true) m_delete = "--delete";

    int remotenum = lNode.nChildNode("remote");
    if (remotenum == 0) {
        perror("error there are no remote servers in your config xml");
        exit(1);
    }
    for (int i = 0; i < remotenum; i++) {
        string temp_ip = lNode.getChildNode("remote", i).getAttribute("ip");
        string temp_module = lNode.getChildNode("remote", i).getAttribute("name");
        if (temp_ip == "" || temp_module == "") {
            perror("error remote servers ip and moudle empty see config xml");
            exit(1);
        }
        rmtServers.push_back(ptrRmtServer(new RemoteServer(temp_ip, temp_module)));
    }

    if (rmtServers.size()) {
        cout << "config xml parse success" << endl;
        cout << "please set /etc/rsyncd.conf max connections=0 Manually" << endl;
        cout << "sersync working thread " << init->sync_num + 2 << "  = " << "1(primary thread) + 1(fail retry thread) + " << init->sync_num \
            << "(daemon sub threads) " << endl;
        cout << "Max threads numbers is: " << 2 + init->sync_num + (rmtServers.size()) * init->sync_num << " = " << 2 + init->sync_num << "(Thread pool nums) + "\
			 << (rmtServers.size())*(init->sync_num) << "(Sub threads)" << endl;
        cout << "please according your cpu ，use -n param to adjust the cpu rate" << endl;
        return 1;
    } else {

        return 0; //fail
    }
}

FileSynchronize::FileSynchronize(ptrInitialize init, ptrQFilter qf, ptrQRetry qr) {
    m_init = init;
    m_qf = qf;
    m_qr = qr; //
    XmlParse(init); //parse xml script
    qr->SetRetryInfo(file_name, time); //设置失败记录脚本路径和执行时间间隔
    if (init->exec_flag & RSYNC_ONCE) {
        FileSynchronize::RsyncOnce();
    } else {
        FileSynchronize::firstflag++;
    }

    ptrInterface itf;
    if (!m_plugin.empty()) {
        if (m_plugin == "refreshCDN") {
            ptrInterface temp(new CdnRefreshInterface());
            itf = temp;
        }
        if (m_plugin == "socket") {
            ptrInterface temp(new SocketInterface());
            itf = temp;
        }
        if (m_plugin == "command") {
            ptrInterface temp(new CommandInterface());
            itf = temp;
        }
        //if (m_plugin == "http")
        //{
        //ptrInterface temp( new HttpInterface( ) );
        //itf = temp;
        //}
        if (itf.use_count() != 0) {
            itf->XmlParse(init->config_file_name);
            watch = itf->m_watch;
        } else {
            cout << "sersync plugin error，please confirm the config xml" << endl;
            exit(1);
        }

    }
    boost::thread_group syncThreads;
    //create threads
    for (int i = 1; i <= init->sync_num; i++) {

        syncThreads.create_thread(boost::bind(&ServerSyncThread, qf, qr, itf));
    }
    //fail retry thread create
    boost::thread retryThread(boost::bind(&QueueRetryThread, qr));
}

void* FileSynchronize::ServerSyncThread(ptrQFilter qf, ptrQRetry qr, ptrInterface itf) {
    while (1) {
        boost::mutex::scoped_lock nlock(worklock);
        sleep_group++;
        while (qf->empty()) {
            work_cond.wait(nlock);
        }
        sleep_group--;
        Event event = qf->pop();
        nlock.unlock();
        boost::thread_group rsync_group;

        for (int offset = 0; offset < rmtServers.size(); offset++) {
            rsync_group.create_thread(boost::bind(&RsyncThread, qr, event, rmtServers[offset]->ip, rmtServers[offset]->module, watch));
        }
        rsync_group.join_all();
        if (itf.use_count() != 0) {
            itf->Execute(event);
        }

    }
}//end ServerSyncThread

void* FileSynchronize::RsyncThread(ptrQRetry qr, Event event, string ip, string module, string watch) {
    string eventpath = event->path;

    int operation = event->operation;
    int dir = event->dir;
    string path;
    string command = "cd " + watch + " && rsync " + params + " -R " + ssh + port + timeout;
    if (operation == 1) // create or modify a file on the remote server
    {
        if (eventpath.empty()) {
            path = ".";
        } else {
            path = "." + eventpath.substr(watch.length(), (eventpath.length() - watch.length()));
        }
        command += StrEscaped(path) + " ";

    } else // delete a file on the remote server
    {
        if (eventpath.empty()) {
            path = "";
        } else {
            path = eventpath.substr(watch.length() + 1, (eventpath.length() - watch.length()));
        }
        path = DelPathCombine(path, dir);
        command += m_delete + " ./ " + path + " ";
    }
    command += m_users + ip;
    if (ssh.empty()) {
        command += "::";
    } else {
        command += ":";
    }
    command += module + m_password;
    if (!debug) command += " >/dev/null 2>&1 ";

    //debug info for user
    if (debug) cout << command << endl;

    int res = system(command.c_str());
    if (debug_level & THREAD_DEBUG) {
        cout << "Rsync command:" << command << endl;
        cout << "res: " << res << endl;
    }

    /*5888 is the error no permitted to  set times  but the file has been rsynced*/
    if (res && res != 5888) qr->push(command);

} //end RsyncThread

void FileSynchronize::ThreadAwaken() {
    while (!m_qf->empty()) {
        boost::mutex::scoped_lock nlock(worklock);
        if (sleep_group > 0) {
            work_cond.notify_one();
        }
        nlock.unlock();
        usleep(1);
    }
}

void* FileSynchronize::QueueRetryThread(ptrQRetry qr) {
    boost::system_time time_start = boost::get_system_time();
    boost::system_time crontabStart = boost::get_system_time();
    while (1) {
        boost::system_time timeout = boost::get_system_time() + boost::posix_time::minutes(1);
        string command;
        bool has_command = qr->time_wait_pop(command, timeout);
        if (!has_command) {
            boost::system_time time_cur = boost::get_system_time();
            boost::posix_time::time_duration offset = time_cur - time_start;
            if (offset.total_seconds() >= qr->retryInterval) {
                ExecuteScript(qr->fileName);
                time_start = boost::get_system_time();
            }
            if (m_crontab > 0) {
                time_cur = boost::get_system_time();
                offset = time_cur - crontabStart;
                if (offset.total_seconds() >= (m_crontab * 60)) {
                    RsyncOnce();
                    crontabStart = boost::get_system_time();
                }
            }
        } else {
            int res = system(command.c_str());
            if (res != 0 && res != 5888)// 5888 is the error no permitted to  set times  but the file has been rsynced
            {
                char temp[64];
                sprintf(temp, "%d", res);
                string s(temp);
                command = "#errno " + s + "\n" + command + "\n";
                qr->ErrorLog(command); // only one reader so safe
            }
        }

    }//end while
}

void FileSynchronize::ExecuteScript(string file_name) {
    string command = "/bin/sh " + file_name;
    if (debug) {
        cout << "execute script: " << endl << command << endl;
    }
    system(command.c_str());
    ofstream out;
    out.open(file_name.c_str(), ofstream::out); //after execute clear the rsync_fail_log.sh
    out.close();
    command = "chmod 777 " + file_name;
    system(command.c_str());
}

/**
 *@param str 同步相对路径前面没有./,如 t1/t2/t3.php
 * @return 返回符合rsync 规则的路径 --include=t1/ --include=t1/t2 --include=t1/t2/t3.php --exclude=*
 */
std::string FileSynchronize::DelPathCombine(std::string path, bool dir) {
    string temp = " ";
    int pos = 0;
    while ((pos = path.find("/", pos)) != string::npos) {
        temp += " --include=" + StrEscaped(path.substr(0, pos + 1));
        pos++;
    }
    temp += " --include=" + StrEscaped(path);

    if (dir == true) temp += "  --include=" + StrEscaped(path + "/***");
    temp += " --exclude=* ";

    return temp;
}

//=======================================================================
//函数名：  StrEscaped
//作者：    zy(zhoukunta@qq.com)
//日期：    2009-04-01
//功能：    将路径中的字符特殊字符转义
//输入参数： path(string) 要转义的路径名
//返回值：  string 转义后的字符串
//修改记录：
//=======================================================================

string FileSynchronize::StrEscaped(std::string path) {
    string temp = "";
    for (int i = 0; i < path.length(); i++) {
        if (path[i] == '$') {
            temp += "\\$";
            continue;
        }
        temp += path[i];
    }
    return "\"" + temp + "\"";
}



//=======================================================================
//name:		RsyncOnce()
//author:	zy(zhoukunta@qq.com)
//date:		2009-04-02
//function:	rsync all the local files to the remote servers
//param:	void
//return:	void
//modify:	if debug start rsync command without </dev/null 2>&1
//=======================================================================
int FileSynchronize::firstflag = 0;

bool FileSynchronize::RsyncOnce() {
    if (0 == firstflag) {
        if (!Initialize::filter.empty()) {
            cout << "************************attention***********************************" << endl;
            cout << "you set the filter so the \"-r \" can not work" << endl;
            firstflag++;
            return false;
        }
        cout << "------------------------------------------" << endl;
        cout << "rsync the directory recursivly to the remote servers once" << endl;
        cout << "working please wait..." << endl;
    }

    for (int i = 0; i < rmtServers.size(); i++) {
        string command = "cd " + watch + " && rsync " + params + " -R " + m_delete + " ./ " + ssh + port + timeout;
        command += m_users + rmtServers[i]->ip;
        if (ssh.empty()) {
            command += "::";
        } else {
            command += ":";
        }
        command += rmtServers[i]->module + m_password;
        command += AddExclude();
        if (!debug) command += " >/dev/null 2>&1 ";
        if (firstflag == 0) {
            cout << "execute command: " << command << endl;
            firstflag++;
        }
        if (debug) cout << "crontab command:" << command << endl;
        system(command.c_str());
    }
}

//=======================================================================
//函数名：  AddExclude
//作者：    zy(zhoukunta@qq.com)
//日期：    2009-04-02
//功能：    加入需要过滤的文件
//输入参数： 无
//返回值：  string 拼好的需要过滤的文件
//修改记录：
//=======================================================================

string FileSynchronize::AddExclude() {
    string command;
    for (int i = 0; i < cfilter.size(); i++) {
        command += " --exclude=" + StrEscaped(cfilter[i]);
    }
    return command;
}
