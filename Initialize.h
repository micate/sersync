/* 
 * File:   Initialize.h
 * Author: root
 *
 * Created on 2010年1月10日, 下午7:55
 */

#ifndef _INITIALIZE_H
#define	_INITIALIZE_H
#include<stdio.h>
#include<boost/shared_ptr.hpp>
#include<string>
#include<vector>
#include <sys/inotify.h>
#define OPEN_DAEMON         0x01
#define RSYNC_ONCE          0x01<<1
#define SYN_NUM             0x01<<2
#define OTHER_CONFNAME      0x01<<3
#define EXECUTE_SCRIPT      0x01<<5
#define SERSYNC_MODULE      0x01<<6
#define REFRESHCDN_MODULE   0x01<<7
#define HTTP_MODULE         0x01<<8
#define SOCKET_MODULE       0x01<<9
#define COMMAND_MODULE      0x01<<10

struct RemoteServer {
    std::string ip;
    std::string module;

    inline RemoteServer(std::string ip = "", std::string module = "") : ip(ip), module(module) {
    }
};
typedef boost::shared_ptr<RemoteServer> ptrRmtServer;

class Initialize {
public:
    int exec_flag;
    int sync_num;
    std::string module;
    std::string config_file_name;
    std::string hostip;
    static std::vector<std::string> filter;
    int port;
    static bool debug;
    bool deleteFlag;
    static bool createFile;
    static bool createFolder;
    static bool xfs;
public:
    Initialize(int argc, char *argv[]);
    void ConfigInotify();
    int XmlParse();
    static std::string SplitLastSlash(std::string str);
protected:
    int CammandParse(int argc, char *argv[]);

};
typedef boost::shared_ptr<Initialize> ptrInitialize;
#endif	/* _INITIALIZE_H */

