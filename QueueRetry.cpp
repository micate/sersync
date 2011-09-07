#include"QueueRetry.h"
#include <iostream>
#include <fstream>
using namespace std;

QueueRetry::QueueRetry() : fileName("rsync_fail_log.sh"), retryInterval(7200)
{

}

void QueueRetry::SetRetryInfo(string file_name, int time)
{
    fileName = file_name;
    retryInterval = time;
    ofstream out;
    out.open(fileName.c_str(), ofstream::app);
    out.close();
    string command = "chmod 777 " + fileName;
    system(command.c_str());
}

bool QueueRetry::empty()
{
    boost::mutex::scoped_lock lock(queue_mutex);
    return que.empty();
}

string QueueRetry::pop()
{
    boost::mutex::scoped_lock lock(queue_mutex);
    while (que.empty())
    {
        cond.wait(lock);
    }
    string command = que.back();
    que.pop_back();
    return command;
}

bool QueueRetry::time_wait_pop(string& command, boost::system_time timeout)
{
    boost::mutex::scoped_lock lock(queue_mutex);
    if (que.empty())
    {
        if (!cond.timed_wait(lock, timeout))
        {
            return false;
        }
    }
    command = que.back();
    que.pop_back();
    return true;
}

void QueueRetry::pop_back()
{
    boost::mutex::scoped_lock lock(queue_mutex);
    while (que.empty())
    {
        cond.wait(lock);
    }
    que.pop_back();
}

string QueueRetry::back()
{
    boost::mutex::scoped_lock lock(queue_mutex);
    while (que.empty())
    {
        cond.wait(lock);
    }
    string command = que.back();
    return command;
}

int QueueRetry::push(string command)
{
    boost::mutex::scoped_lock lock(queue_mutex);
    if (que.size() >= MAX_LENGTH_FAILQUEUE)
    {
        ErrorLog(command);
        return 0;
    }
    cond.notify_all();
    que.push_front(command);
    return 1;
}

void QueueRetry::ErrorLog(string command)
{
    ofstream out;
    out.open(fileName.c_str(), ofstream::app);
    out << command << endl;
    out.close();
}

void QueueRetry::printdeque()
{
    boost::mutex::scoped_lock lock(queue_mutex);
    int cre = 0;
    int del = 0;
    if (que.empty())
    {
        cout << "empty retry Queue" << endl;
    } else
    {
        for (std::deque<string>::iterator itrt = que.begin(); itrt < que.end(); itrt++)
        {
            std::cout << "Queue Retry: " << (*itrt) << "\n";
            if ((*itrt).find("--delete") != string::npos)
            {
                del++;
            } else
            {
                cre++;
            }
        }
        cout << "Queue Retry: " << endl;
        cout << "amount: " << que.size() << "\t";
        cout << "create: " << cre << "\t";
        cout << "delete: " << del << "\t";
        std::cout << endl << endl << endl;
    }

}
