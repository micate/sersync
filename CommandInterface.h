/* 
 * File:   CommandInterface.h
 * Author: root
 *
 * Created on 2010年5月13日, 下午8:47
 */

#ifndef _COMMANDINTERFACE_H
#define	_COMMANDINTERFACE_H
#include "Interface.h"
#include "Inotify.h"

class CommandInterface : public Interface {
public:
    std::string m_prefix;
    std::string m_suffix;
    std::vector<ptrRegex> pattern;
    bool m_ignoreError;
    int m_debug;
    bool m_filter;
public:
    CommandInterface();
    virtual void XmlParse(std::string config_file_name);
    virtual int Execute(Event e);
};

#endif	/* _COMMANDINTERFACE_H */

