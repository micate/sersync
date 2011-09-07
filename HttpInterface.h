/* 
 * File:   HttpInterface.h
 * Author: zy
 *
 * Created on 2010年1月23日, 下午11:57
 */

#ifndef _HTTPINTERFACE_H
#define	_HTTPINTERFACE_H
#include"Interface.h"

class HttpInterface : public Interface {
public:
    std::string m_url;
public:
    HttpInterface();
    virtual void XmlParse(std::string config_file_name);
    virtual int Execute(Event e);
};

#endif	/* _HTTPINTERFACE_H */

