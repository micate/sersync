/* 
 * File:   QueueFilter.h
 * Author: root
 *
 * Created on 2009年11月26日, 上午7:12
 */
#ifndef _QUEUEFILTER_H
#define	_QUEUEFILTER_H
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include<deque>

using namespace std;

struct InotifyEvent;
typedef boost::shared_ptr<InotifyEvent> Event;

class QueueFilter {
private:
    boost::mutex queue_mutex;
    boost::condition_variable con;
    std::deque<Event> que;
public:
    QueueFilter();
    int push(Event e);
    Event pop();
    void printdeque();
    bool empty();
private:
    int filter(Event e);
};
typedef boost::shared_ptr<QueueFilter> ptrQFilter;
#endif	/* _QUEUEFILTER_H */

