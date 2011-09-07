/* 
 * File:   CdnRefresh.h
 * Author: 周洋 JohnTech
 *
 * Created on 2010年1月19日, 下午3:29
 */
#ifndef _CDNREFRESH_H
#define	_CDNREFRESH_H
#include "Inotify.h"
#include"Interface.h"
#include<fstream>
using namespace std;

class CdnRefreshInterface : public Interface {
public:
    string m_domainName;
    string m_username;
    string m_passwd;
    string m_regexFlag; //if == 'false' will ignore the regex match;
    string m_urlBase;
    string m_urlRegex;
    ofstream m_fout;
public:
    CdnRefreshInterface();
    virtual void XmlParse(std::string xml = "confxml.xml");
    virtual int Execute(Event e);
protected:
    string PackagePath(Event e);
    void ErrorLog(std::string temp,std::string fullPath);
};

#endif	/* _CDNREFRESH_H */

