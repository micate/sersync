/* 
 * File:   Interface.h
 * Author: root
 *
 * Created on 2010年1月24日, 下午6:02
 */

#ifndef _INTERFACE_H
#define	_INTERFACE_H
#include<time.h>
#include<string.h>
#include"Inotify.h"

class Interface {
public:
    int m_port;
    struct timeval timeout;
    std::string m_watch;
public:
    Interface();
    virtual void XmlParse(std::string config_file_name) = 0;
    virtual int Execute(Event e) = 0;
};

typedef boost::shared_ptr<Interface> ptrInterface;
#endif	/* _INTERFACE_H */

