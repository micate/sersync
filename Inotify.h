#include <string>
#include<iostream>
#include <sys/inotify.h>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <deque>
#include <map>
#include "Initialize.h"
#include"QueueFilter.h"

#ifndef _INOTIFY_H
#define _INOTIFY_H
//需要监控的事件


struct InotifyEvent {
    int wd; //watch descriptor for the function named  inotify_add_watch
    std::string path;
    bool dir; // true if is dir;
    int mask; // event mask  response to IN_SYNC
    int operation; // create/delete = 1/0
};

typedef std::map<int, std::string> PathMap;
typedef std::pair<int, std::string> PathPair;
typedef boost::shared_ptr<InotifyEvent> Event;
typedef boost::shared_ptr<boost::regex> ptrRegex;

/**
 * Inotify 提供对指定目录的完整递归监控及对监控的管理。
 *
 */
class Inotify {
protected:
    PathMap m_path; // source under watching
    int m_fd; //watch resource identifier
    string m_watch;
    static std::vector<ptrRegex> pattern;
    static int m_first_add_watch;
    static int inotify_read_times;
public:
    Inotify(std::string rootPath); //add root path which want to watch
    bool AddWatch(std::string path);
    bool RemoveWatch(std::string path); //remove watch path
    unsigned int GetEvents(ptrQFilter que);
protected:
    int FilterEvent(inotify_event* event); //filt redundant or error event
    int FilterExclude(inotify_event* event); // filt the exclude directory or file
    int FillEvent(inotify_event* event, ptrQFilter que);
    static void* InitReguar();
    bool IsXfsDir(std::string path);
private:
    bool ReadFill(ptrQFilter que);
};
#endif
