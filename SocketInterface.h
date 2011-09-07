/* 
 * File:   SocketInterface.h
 * Author: root
 *
 * Created on 2010年1月23日, 下午11:59
 */

#ifndef _SOCKETINTERFACE_H
#define	_SOCKETINTERFACE_H
#include"Interface.h"

class SocketInterface : public Interface {
public:
    std::string m_ip;
public:
    SocketInterface();
    virtual void XmlParse(std::string config_file_name);
    virtual int Execute(Event e);
};

#endif	/* _SOCKETINTERFACE_H */

