#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<stdio.h>
#include <deque>
#include<sys/select.h>
#include<sys/time.h>
#include <stdlib.h>
#include<string.h>
#include<fcntl.h>
#include "Initialize.h"
#include "Inotify.h"
#include"main.h"

//最大缓冲数量
#define MAX_BUF_SIZE 5120
bool printEvent(inotify_event *e);
int flag; //store inotify resource descriptor status;
struct timeval tv;
fd_set rfds;
extern int debug_level;
extern int IN_SYNC;
using namespace std;
std::vector<ptrRegex> Inotify::pattern;
int Inotify::m_first_add_watch = 1;

Inotify::Inotify(string rootPath) {
    //初始化正则表达式对象
    Inotify::InitReguar();
    m_fd = inotify_init();
    m_watch = rootPath; //do not have the '/' tail;
    AddWatch(rootPath);
    m_first_add_watch = 0;
    flag = fcntl(m_fd, F_GETFL, 0);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    FD_ZERO(&rfds);
    FD_SET(m_fd, &rfds);
}

//=======================================================================
//函数名：  InitReguar
//作者：    zy(zhoukunta@qq.com)
//日期：    2009-04-05
//功能：    初始化正则表达式对象
//输入参数：无
//返回值：  无
//修改记录：
//=======================================================================

void* Inotify::InitReguar() {
    for (int i = 0; i < Initialize::filter.size(); i++) {
        try {
            ptrRegex tmp(new boost::regex((Initialize::filter[i])));
            pattern.push_back(tmp);
        } catch (boost::regex_error& e) {
            cout << endl << "filter 中" << Initialize::filter[i] << "regular expression error,filter fail" << endl;
            cout << "error content:\t" << e.what() << endl;
            continue;
        }
    }
}

int Inotify::inotify_read_times = 3;

unsigned int Inotify::GetEvents(ptrQFilter q) {
    int times = 1;
    int again = ReadFill(q);
    fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
    while (again && (times < inotify_read_times)) {
        FD_ZERO(&rfds);
        FD_SET(m_fd, &rfds);
        int retval = select(m_fd + 1, &rfds, NULL, NULL, &tv);
        if (retval) again = ReadFill(q);
        times++;
    }
    fcntl(m_fd, F_SETFL, flag);
    return 1;
}
//=======================================================================
//function name：  ReadFill
//author：    zy(zhoukunta@qq.com)
//date：    2009-01-24
//function：    将读到的一个或多个inotify事件填入队列
//imput params：q(ptrQFilter)队列的指针对象
//
//return：  true有事件填入 false无事件填入
//=======================================================================

bool Inotify::ReadFill(ptrQFilter q) {
    int len = 0;
    char buffer[MAX_BUF_SIZE];
    char* offset = buffer;
    len = read(m_fd, buffer, MAX_BUF_SIZE); //read May lead to this programme blocking
    if (len == -1) return false; //inotify queue empty
    while (offset - buffer < len) {
        inotify_event* event = (struct inotify_event*) offset;
        offset += sizeof ( struct inotify_event) +event->len;
        if (Initialize::debug == true) {
            cout << "inotify wd:" << event->wd << "\tname:" << event->name << "\tmask:" << event->mask << endl;
        }
        if (FilterEvent(event)) continue; //if tmp file continue;
        if (!pattern.empty() && FilterExclude(event)) continue; //filter match regular pattern in xml
        if (debug_level & INOTIFY_DEBUG) printEvent(event);
        FillEvent(event, q);
    }
    return true;
}

int Inotify::FillEvent(inotify_event* event, ptrQFilter que) {

    Event tempEvent(new InotifyEvent());
    tempEvent->wd = event->wd;
    tempEvent->path = (m_path[ event->wd ] + "/" + event->name); //sys sometimes produce large ie.wd which we don't add in the wd list
    tempEvent->mask = event->mask;

    if (event->mask & IN_IGNORED) {
        tempEvent->operation = -1;
        return 0;
    } else if ((event->mask & IN_MOVED_FROM) || (event->mask & IN_DELETE)) {
        tempEvent->operation = 0;
    } else if (
            (event->mask & IN_CLOSE_WRITE) ||
            (event->mask & IN_MODIFY) ||
            (event->mask & IN_CREATE) ||
            (event->mask & IN_MOVED_TO)) {
        tempEvent->operation = 1;
    }

    if (event->mask & IN_ISDIR)//remove or delete directory
    {
        tempEvent->dir = true;
        if (tempEvent->operation == 0) //delete folder
        {
            RemoveWatch(tempEvent->path);
        } else if (tempEvent->operation == 1)//create folder add watch
        {
            AddWatch(tempEvent->path);
        }
    } else {
        tempEvent->dir = false;
    }
    //add to rsync queue
    que->push(tempEvent);
    return 1;
}


//=======================================================================
//function name： AddWatch
//author：       zy(zhoukunta@qq.com)
//date：         2010-06-02
//function：     add the path to inotify api
//input params： path(string) the path need to watch
//return：       bool true if success else false
//=======================================================================

bool Inotify::AddWatch(std::string path) {
    int wd = inotify_add_watch(m_fd, path.c_str(), IN_SYNC);
    if (-1 == wd) {
        perror("inotify_add_watch error");
        return false;
    }

    if (Initialize::debug == true) {
        cout << "add watch: " << path << " return wd is: " << wd << endl;
    }

    m_path.insert(PathPair(wd, path));
    DIR* pdir = NULL;
    struct dirent *pfile = NULL;
    if (!(pdir = opendir(path.c_str()))) return false;

    while ((pfile = readdir(pdir))) //read directory content and add recursively
    {
        string pre = path + "/" + pfile->d_name;
        bool is_dir = false;
        if (Initialize::xfs == true) {
            is_dir = IsXfsDir(pre);
        } else {
            is_dir = (pfile->d_type == 4);
        }
        if (is_dir && strcmp(pfile->d_name, ".") && strcmp(pfile->d_name, "..")) {
            if (m_first_add_watch) {
                int length = pre.size() - (m_watch.size() + 1); //relative directory length m_watch+'/'
                string tmp = pre.substr(m_watch.size() + 1, length) + "/";
                boost::cmatch what;
                int filter = 0;
                for (int i = 0; i < pattern.size(); i++) {
                    if (boost::regex_match(tmp.c_str(), what, *(pattern[i]))) {
                        filter = 1;
                        break;
                    }
                }
                if (filter) continue;
            }
            AddWatch(pre);
        }
    }
    closedir(pdir);
    return true;
}

bool Inotify::IsXfsDir(std::string path) {
    struct stat sb;
    if (stat(path.c_str(), &sb) == -1) return false;
    return S_ISDIR(sb.st_mode);
}

/*
 *input params: string  path not end up with slash /opt/tongbu/temp
 *function:     remove the path from watching deque
 *return：      success true else false;
 */
bool Inotify::RemoveWatch(std::string path) {
    bool finded = false;
    PathMap::iterator i = m_path.begin();
    for (; i != m_path.end();) {
        if (i->second.empty()) {
            ++i;
        } else if (i->second.substr(0, path.length()) == path) //查找已经添加的监控
        {
            if ((i->second[ path.length() ]) == '/' || i->second[ path.length() ] == 0)//ignore the error /1234/123/  1234/12/ as the same directory tree
            {
                inotify_rm_watch(m_fd, i->first);
                m_path.erase(i++);
                finded = true; //找到并删除
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }
    if (!finded) return false;
    return true;
}

//=======================================================================
//函数名：  FilterExclude
//作者：    zy(zhoukunta@qq.com)
//date：    2009-04-05
//function：    filter Exclude path and file
//输入参数：offset(struct inotify_event*)
//
//return：  1该事件需要过滤 0该事件不需过滤
//=======================================================================

int Inotify::FilterExclude(struct inotify_event* offset) {
    int isfilter = 0;
    boost::cmatch what;
    string tmp = m_path[ offset->wd ];
    string pre = "";
    int length;
    if ((length = (tmp.size() - m_watch.size())) > 0) {
        pre = tmp.substr(m_watch.size(), length);
        pre = pre + "/" + offset->name;
    } else {
        pre = offset->name; //root path
    }
    for (int i = 0; i < pattern.size(); i++) {
        if (boost::regex_match(pre.c_str(), what, *(pattern[i]))) return 1;
    }

    return 0;
}

int Inotify::FilterEvent(struct inotify_event* offset) {
    int isfilter = 0;
    do {
        isfilter = (NULL == offset);
        if (isfilter) break;
        //filter create file Event
        isfilter = (Initialize::createFile == false) && ((offset->mask & IN_ISDIR) == 0) && (offset->mask & IN_CREATE);
        if (isfilter) break;

        isfilter = (Initialize::createFolder == false) && ((offset->mask & IN_ISDIR)) && (offset->mask & IN_CREATE);
        if (isfilter) break;

        isfilter = ((strcmp(offset->name, "4913") == 0) && ((offset->mask & IN_ISDIR) == 0));
        isfilter = isfilter || (strcmp((offset->name), "") == 0);
        if (isfilter) break;

        int length = strlen((offset) ->name);
        isfilter = (((offset->name[ length - 1 ] == '~') || ((offset) ->name[ 0 ] == '.')) && (!(offset->mask & IN_ISDIR)));
        if (isfilter) break;


        isfilter = (0 == offset->wd || 0 == offset->mask);
        if (isfilter) break;

        PathMap::reverse_iterator last = m_path.rbegin();
        isfilter = ((offset->wd) > (last->first));
        if (isfilter) break;

        isfilter = (Initialize::createFile == false) && ((offset->mask & IN_ISDIR) == 0) && (offset->mask & IN_CREATE);
        if (isfilter) break;

        isfilter = (Initialize::createFolder == false) && (offset->mask & IN_ISDIR) && (offset->mask & IN_CREATE);
        if (isfilter) break;

    } while (0);

    return isfilter;
}

bool printEvent(inotify_event *e) {
    if (debug_level & SUB_CLASS) {
        cout << "name: " << e->name << "\t" << "mask: " << e->mask << "\t" << "len: " << e->len << endl;
    }

}








