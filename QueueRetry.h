/* 
 * File:   QueueRetry.h
 * Author: root
 *
 * Created on 2009年12月3日, 上午4:45
 */

#ifndef _QUEUERETRY_H
#define	_QUEUERETRY_H
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include<deque>
using namespace std;
#define MAX_LENGTH_FAILQUEUE  5000

class QueueRetry {
public:
    string fileName; //error log Name
    int retryInterval;
private:
    boost::mutex queue_mutex;
    boost::condition_variable cond;
    std::deque<string> que;

public:
    QueueRetry();
    string pop();
    bool time_wait_pop(string& command, boost::system_time timeout);
    int push(string fe);
    bool empty();
    void printdeque();
    string back();
    void pop_back();
    void ErrorLog(string command);
    void SetRetryInfo(string file_name ,int time);
};
typedef boost::shared_ptr<QueueRetry> ptrQRetry;


#endif	/* _QUEUERETRY_H */

