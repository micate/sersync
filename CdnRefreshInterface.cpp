#include <errno.h>
#include<arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <cstdlib>
#include <errno.h>
#include <string.h>
#include<string>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <boost/regex.hpp>
#include<fcntl.h>
#include<sys/time.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<fstream>
#include <boost/regex.hpp>
#include "xmlParser.h"
#include "CdnRefreshInterface.h"
#include "Initialize.h"
#include"main.h"

#define MAXDATASIZE    4096 /*the maximum number of every transfer*/

CdnRefreshInterface::CdnRefreshInterface()
{
	//设定超时重传时间
	timeout.tv_sec = 0;
	timeout.tv_usec = 300000;
}

void CdnRefreshInterface::XmlParse(std::string config_file_name)
{
	XMLNode xMainNode = XMLNode::openFileHelper( config_file_name.c_str( ), "head" );
	int num = xMainNode.nChildNode( "plugin" );
	if (num == 0)
	{
		perror( "错误，配置文件中没有插件标签\n" );
		exit( 1 );
	}
	for (int i = 0; i < num; i++)
	{
		string name = xMainNode.getChildNode( "plugin", i ).getAttribute( "name" );
		if (name != "refreshCDN")
		{
			continue;
		}
		XMLNode xNode = xMainNode.getChildNode( "plugin", i );
		XMLNode lNode = xNode.getChildNode( "localpath", 0 );
		int wtchnum = xNode.nChildNode( "localpath" ); //get the numbers of the localpath tag
		if (wtchnum == 0)
		{
			perror( "错误，配置文件中没有指明需要监控的路径\n" );
			exit( 1 );
		}
		m_watch = lNode.getAttribute( "watch" );
		m_watch = Initialize::SplitLastSlash( m_watch );

		XMLNode Node = lNode.getChildNode( "cdninfo" );
		m_domainName = Node.getAttribute( "domainname" );
		m_port = atoi( Node.getAttribute( "port" ) );
		m_username = Node.getAttribute( "username" );
		m_passwd = Node.getAttribute( "passwd" );

		Node = lNode.getChildNode( "sendurl" );
		m_urlBase = Node.getAttribute( "base" );

		Node = lNode.getChildNode( "regexurl" );
		m_urlRegex = Node.getAttribute( "match" );
		m_regexFlag = Node.getAttribute( "regex" ); //if =='false'will ignore the regex match;

		if (m_domainName.empty( ) || m_username.empty( ) || m_passwd.empty( ) ||   \
		m_urlBase.empty( ) || m_regexFlag.empty( ) || m_urlRegex.empty( )  \
		)
		{

			cout << "配置文件错误，一些xml属性不应为空，请检查" << endl;
			exit( 1 );
		}
		break;
	}


}

int CdnRefreshInterface::Execute(Event e)
{

	string fullPath = PackagePath( e );
	if(fullPath.empty())
	{
		return 1;
	}
	if (debug_level & SUB_CLASS)
	{
		cout << fullPath << endl;
	}

	char buf[MAXDATASIZE] = "\0";//receive date from cdn
	int sockfd, recvbytes, flags, n;
	struct sockaddr_in serv_addr;
	struct hostent *host;

	if ((host = gethostbyname(  m_domainName.c_str() )) == NULL)
	{
		printf( "get host by name error ccms.chinacache.com down" );
		return 1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons( m_port );
	serv_addr.sin_addr = *((struct in_addr *) host->h_addr);
	bzero( &(serv_addr.sin_zero), 8 );

	if ((sockfd = socket( AF_INET, SOCK_STREAM, 0 )) == -1)
	{
		return 1;
	}
	flags = fcntl( sockfd, F_GETFL, 0 );
	fcntl( sockfd, F_SETFL, flags | O_NONBLOCK );

	connect( sockfd, (struct sockaddr *) & serv_addr, sizeof (struct sockaddr) );

	fd_set rset, wset;
	FD_ZERO( &wset );
	FD_SET( sockfd, &wset );

	if ((n = select( sockfd + 1, NULL, &wset, NULL, &timeout )) <= 0)
	{
		if (debug_level & SUB_CLASS)
		{
			printf( "time out connect error" );
		}
		close( sockfd );
		return 1;
	}

	if (send( sockfd, fullPath.c_str( ), strlen( fullPath.c_str( ) ), 0 ) == -1)
	{
		if (debug_level & SUB_CLASS)
		{
			cout << "send error!";
		}
		close( sockfd );
		return 1;
	}

	//需要重置，因为之前select之后，会对相应的rset置为非0
	FD_ZERO( &rset );
	FD_SET( sockfd, &rset );
	if ((n = select( sockfd + 1, &rset, NULL, NULL, &timeout )) <= 0)
	{
		if (debug_level & SUB_CLASS)
		{
			printf( "time out connect error" );
		}
		close( sockfd );
		return 1;
	}
	recvbytes = recv( sockfd, buf, MAXDATASIZE, 0 );
	if (0 == recvbytes || -1 == recvbytes)
	{
		if (debug_level & SUB_CLASS)
		{
			cout << "this recvbytes==0" << endl;
		}
		close( sockfd );
		return 1;
	}

	ErrorLog( buf ,fullPath);
	close( sockfd );
	if (debug_level & SUB_CLASS)
	{
		printf( "Received: %s\n", buf ); /*从返回结果读取 whatsup信息,如果为succeed说明提交成功，另超量提交类似获取 */
	}
	return 0;

}

string CdnRefreshInterface::PackagePath(Event e)
{
	string temp;
	char buf[1024] = "\0";
	string fullPath;
	fullPath += m_urlBase;
	if (m_regexFlag == "true")
	{
		boost::smatch what;
		boost::regex expression( m_urlRegex );
		int res = boost::regex_search( e->path, what, expression );
		if (false == res || what.size( ) <= 0)
		{
			return temp;
		}
		fullPath += what[1];
		fullPath += what.suffix( );
	} else
	{
		string tp = (e->path).substr( m_watch.size( ), (e->path).size( ) - m_watch.size( ) );
		fullPath += tp;
	}

	if (e->dir)
	{
		fullPath = "&dirs=" + fullPath + "/";
	} else
	{
		fullPath = "&urls=" + fullPath;
	}
	snprintf( buf, sizeof (buf),    \
			"HEAD /index.jsp?user=%s&pswd=%s&ok=ok%s HTTP/1.0\r\nHost: %s\r\n\r\n",    \
			m_username.c_str( ), m_passwd.c_str( ), fullPath.c_str( ), m_domainName.c_str( ) );

	temp = buf;
	return temp;
}

void CdnRefreshInterface::ErrorLog(string temp,string fullPath)
{
	static int iWriteTime = 0;
	if (iWriteTime < 1000)
	{
		m_fout.open( "errorlog.txt", ofstream::app );
		iWriteTime++;
	} else
	{
		m_fout.open( "errorlog.txt", ofstream::out );
		iWriteTime = 0;
	}
	int pos = 0;
	if ((pos = (temp.find( "\n", temp.find( "whatsup" ) ))) != string::npos)
	{
		int i = pos - 1 - temp.find( "whatsup" );
		temp = temp.substr( temp.find( "whatsup" ), i );
	}

	m_fout << temp << " " << fullPath << "\n";
	m_fout.close( );
}

