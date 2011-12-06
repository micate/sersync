#include"QueueFilter.h"
#include "Inotify.h"
#include"main.h"
using namespace std;
#define MAX_SEARCH_LENGTH 10

QueueFilter::QueueFilter( )
{

}

Event QueueFilter::pop( )
{
    boost::mutex::scoped_lock lock(queue_mutex);
    Event event = que.back();
    que.pop_back();
    return event;
}

bool QueueFilter::empty( )
{
    boost::mutex::scoped_lock lock(queue_mutex);
    return que.empty();
}

int QueueFilter::push( Event event )
{
    boost::mutex::scoped_lock lock(queue_mutex);
    if( !filter(event) )
    {
        que.push_front(event);
        return 1;
    }
    return 0;
}

int QueueFilter::filter( Event event )
{

    int searchDistance = 0;
    if( event->dir == true && event->operation == 0 )
    {
        std::deque<Event>::iterator i = que.begin();
        string dirPath = event->path + "/"; //in case this path name is the prefix of other file name
        while( i < que.end() )
        {
            if(((*i)->path).empty()){
                ++i;
                continue;
            }
            string temp = ((*i)->path).substr(0, dirPath.length());
            if( ((*i)->path).substr(0, dirPath.length()) == dirPath )
            {
                if( debug_level & SUB_CLASS )
                {
                    cout << "erase now" << "\t" << (*i)->path << endl;
                }
                i = que.erase(i);
                continue;
            }
            i++;
        }
    }
    searchDistance = 0;
    std::deque<Event>::iterator i = que.begin();
    while( i < que.end() && searchDistance <= MAX_SEARCH_LENGTH )
    {
        if( (*i)->path == event->path )
        {
            (*i)->operation = event->operation;
            break;
        }
        i++;
        searchDistance++;
    }
    if( i != que.end() && searchDistance < MAX_SEARCH_LENGTH )
    {
        return 1;
    }
    return 0;
}

void QueueFilter::printdeque( )
{
    boost::mutex::scoped_lock lock(queue_mutex);
    int cre = 0;
    int del = 0;
    for( std::deque<Event>::iterator itrt = que.begin(); itrt < que.end(); itrt++ )
    {
        std::cout << "queue filter" << (*itrt)->path << "\t";
        cout << (*itrt)->operation << endl;
        if( (*itrt)->operation == 1 )
        {
            cre++;
        }
        if( (*itrt)->operation == 0 )
        {
            del++;
        }
    }
    if( que.empty() )
    {
        cout << "empty Filter Queue" << endl;
    }else
    {
        cout << "Queue Filter" << "\t";
        cout << "amount: " << que.size() << "\t";
        cout << "create: " << cre << "\t";
        cout << "delete: " << del << "\t";
        std::cout << endl << endl << "\t";
    }
}
