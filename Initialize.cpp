#include <vector>
#include<iostream>
#include<stdlib.h>
#include "Initialize.h"
#include "xmlParser.h"
#include"main.h"
using namespace std;

extern int optind;
extern int optopt;
extern char *optarg;
extern int IN_SYNC;
#define DEFAULT_THREAD_NUM 10
int getopt(int argc, char * const *argv, const char *optstring);

std::vector<string> Initialize::filter;
bool Initialize::debug = false;
bool Initialize::createFile = false;
bool Initialize::createFolder = false;
bool Initialize::xfs = false;

Initialize::Initialize(int argc, char *argv[]) : exec_flag(0), config_file_name("confxml.xml"), sync_num(0), module("sersync")
{
	ConfigInotify(); //modify system param
	CammandParse(argc, argv); //parse command param
	XmlParse();
}

int Initialize::XmlParse()
{
	cout << "parse xml config file" << endl;
	XMLNode xMainNode = XMLNode::openFileHelper(config_file_name.c_str(), "head");
	XMLNode xNode = xMainNode.getChildNode("host");

	hostip = xNode.getAttribute("hostip");
	cout << "host ip : " << hostip << "\t";

	port = atoi(xNode.getAttribute("port"));
	cout << "host port: " << port << endl;

	xNode = xMainNode.getChildNode("filter");
	string temp = xNode.getAttribute("start");
	if ( temp == "true" )
	{
		cout << "now the filter work ,if you set the crontab,you have to set crontab filter " << endl;
		int num = xNode.nChildNode("exclude");
		for ( int i = 0; i < num; i++ )
		{
			string t = xNode.getChildNode(i).getAttribute("expression");
			filter.push_back(t);
		}
	}

	xNode = xMainNode.getChildNode("inotify").getChildNode("delete");
	temp = xNode.getAttribute("start");
	if ( temp == "true" )
	{
		IN_SYNC |= IN_DELETE;
		deleteFlag = true;
	} else
	{
		cout << "will ignore the inotify delete event" << endl;
	}

	temp = xMainNode.getChildNode("inotify").getChildNode("createFile").getAttribute("start");
	if ( temp == "true" )
	{
		IN_SYNC |= IN_CREATE;
		createFile = true;
		cout << "will ignore the inotify createFile event " << endl;
	}

	temp = xMainNode.getChildNode("inotify").getChildNode("createFolder").getAttribute("start");
	if ( temp == "true" )
	{
		IN_SYNC |= IN_CREATE;
		createFolder = true;
	}

	temp = xMainNode.getChildNode("inotify").getChildNode("closeWrite").getAttribute("start");
	if ( temp == "true" ) IN_SYNC |= IN_CLOSE_WRITE;

	temp = xMainNode.getChildNode("inotify").getChildNode("moveFrom").getAttribute("start");
	if ( temp == "true" ) IN_SYNC |= IN_MOVED_FROM;

	temp = xMainNode.getChildNode("inotify").getChildNode("moveTo").getAttribute("start");
	if ( temp == "true" ) IN_SYNC |= IN_MOVED_TO;

	temp = xMainNode.getChildNode("inotify").getChildNode("attrib").getAttribute("start");
	if ( temp == "true" ) IN_SYNC |= IN_ATTRIB;

	temp = xMainNode.getChildNode("inotify").getChildNode("modify").getAttribute("start");
	if ( temp == "true" ) IN_SYNC |= IN_MODIFY;

	
	xNode = xMainNode.getChildNode("debug");
	temp = xNode.getAttribute("start");
	if ( temp == "true" )
	{
		debug = 1;
		cout << "Open debug, you will see debug infomation " << endl;
	}

	xNode = xMainNode.getChildNode("fileSystem");
	temp = xNode.getAttribute("xfs");
	if ( temp == "true" )
	{
		xfs = true;
		cout << "WARNING XFS FILE SYSTEM WORK" << endl;
	}

}

int Initialize::CammandParse(int argc, char* argv[])
{
	optarg = NULL;
	string temp;
	int ch;
	cout << "parse the command param" << endl;
	while ((ch = getopt(argc, argv, "hdro:n:m:")) != -1)
	{

		switch (ch)
		{
			case 'd':
				printf("option: -d \t");
				printf("run as a daemon\n");
				exec_flag |= OPEN_DAEMON;
				break;
			case 'r':
				printf("option: -r \t");
				printf("rsync all the local files to the remote servers before the sersync work\n");
				exec_flag |= RSYNC_ONCE;
				break;
			case 'o':
				printf("option: -o \t");
				printf("config xml name：  %s\n", optarg);
				config_file_name = optarg;
				if ( config_file_name.empty() )
				{
					cout << "ERROR config file name empty" << endl;
					exit(1);
				}
				exec_flag |= OTHER_CONFNAME;
				break;
			case 'm':
				printf("option: -m \t");
				printf("working plugin name: %s\n", optarg);
				module = "other";
				temp = optarg;
				if ( temp == "refreshCDN" )
				{
					exec_flag |= REFRESHCDN_MODULE;
				} else if ( temp == "socket" )
				{
					exec_flag |= SOCKET_MODULE;
				} else if ( temp == "http" )
				{
					exec_flag |= HTTP_MODULE;
				} else if ( temp == "command" )
				{
					exec_flag |= COMMAND_MODULE;
				}
				break;
			case 'h':
				cout << "_______________________________________________________" << endl;
				cout << "参数-d:启用守护进程模式" << endl;
				cout << "参数-r:在监控前，将监控目录与远程主机用rsync命令推送一遍" << endl;
				cout << "c参数-n: 指定开启守护线程的数量，默认为10个" << endl;
				cout << "参数-o:指定配置文件，默认使用confxml.xml文件" << endl;
				cout << "参数-m:单独启用其他模块，使用 -m refreshCDN 开启刷新CDN模块" << endl;
				cout << "参数-m:单独启用其他模块，使用 -m socket 开启socket模块" << endl;
				cout << "参数-m:单独启用其他模块，使用 -m http 开启http模块" << endl;
				cout << "不加-m参数，则默认执行同步程序" << endl;
				cout << "________________________________________________________________" << endl;
				exit(0);
			case 'n':
				printf("option: -n \t");
				printf("thread num is：  %s\n", optarg);
				sync_num = atoi(optarg);
				exec_flag |= SYN_NUM;
				break;
			case '?':
				printf("命令行参数错误: 没有该选项-%c\n", (char) optopt);
				exit(1);
		}
	}//end while
	if ( sync_num == 0 )
	{
		sync_num = DEFAULT_THREAD_NUM;
		cout << "daemon thread num: " << DEFAULT_THREAD_NUM << endl;
	}
}

void Initialize::ConfigInotify()
{
	cout << "set the system param" << endl;
	cout << "execute：echo 50000000 > /proc/sys/fs/inotify/max_user_watches" << endl;
	system("echo 50000000 > /proc/sys/fs/inotify/max_user_watches");
	cout << "execute：echo 327679 > /proc/sys/fs/inotify/max_queued_events" << endl;
	system("echo 327679 > /proc/sys/fs/inotify/max_queued_events");

}

string Initialize::SplitLastSlash(string str)
{
	if ( str.empty() )
	{
		perror("the path your watch don't exists");
		exit(1);
	}
	if ( str.find_last_of("/") == (str.length() - 1) )
	{
		str = str.substr(0, str.length() - 1);
	}
	return str;
}

