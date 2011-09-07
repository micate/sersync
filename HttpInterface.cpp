#include<curl/curl.h>
#include<sstream>
#include "QueueFilter.h"
#include "Initialize.h"
#include "xmlParser.h"
#include "Inotify.h"
#include"HttpInterface.h"

HttpInterface::HttpInterface() : m_url("http://localhost")
{

}

void HttpInterface::XmlParse(std::string config_file_name)
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
		if (name == "http")
		{
			XMLNode xNode = xMainNode.getChildNode( "plugin", i );
			m_watch = xNode.getChildNode( "localpath" ).getAttribute( "watch" );
			m_url = xNode.getChildNode( "localpath" ).getChildNode( "deshost" ).getAttribute( "url" );
			m_port = atoi( xNode.getChildNode( "localpath" ).getChildNode( "deshost" ).getAttribute( "port" ) );
			cout << "监控路径: " << m_watch << "  URL:  " << m_url << " 端口号: " << m_port << endl;
			break;
		}
	}
}

int HttpInterface::Execute(Event e)
{
	CURL *curl;
	CURLM *multi_handle;
	int still_running;
	int times = 0; //try times if select false
	int TRY_TIMES = 50;
	char strMask[10];
	sprintf( strMask, "%d", e->mask );
	string sm = strMask;
	string sendBuffer = "watchroot=" + m_watch;
	if (e->dir)
	{
		sendBuffer += "&isdir=1";
	} else
	{
		sendBuffer += "&isdir=0";
	}
	sendBuffer += "&inotifyPath=" + e->path;
	sendBuffer += "&mask=" + sm;

	curl = curl_easy_init( );
	multi_handle = curl_multi_init( );
	if (curl && multi_handle)
	{
		/* what URL that receives this POST */
		curl_easy_setopt( curl, CURLOPT_URL, m_url.c_str( ) );
		curl_easy_setopt( curl, CURLOPT_HTTPPOST, 1 );
		curl_easy_setopt( curl, CURLOPT_POSTFIELDS, sendBuffer.c_str( ) );
		curl_multi_add_handle( multi_handle, curl );
		while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform( multi_handle, &still_running ));
		while (still_running && times < TRY_TIMES)
		{
			int rc; //select() return code
			int maxfd;
			fd_set fdread;
			fd_set fdwrite;
			fd_set fdexcep;
			FD_ZERO( &fdread );
			FD_ZERO( &fdwrite );
			FD_ZERO( &fdexcep );
			//get file descriptors from the transfers
			curl_multi_fdset( multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd );
			rc = select( maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout );
			switch (rc)
			{
				case -1://select error
					break;
				case 0:
				default: // timeout
					while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform( multi_handle, &still_running ));
					break;
			}
			times++;
		}//end while
		curl_multi_remove_handle( multi_handle, curl );
		curl_easy_cleanup( curl );
		curl_multi_cleanup( multi_handle ); //always cleanup
		if (times >= TRY_TIMES)
		{
			return 1;
		}
		return 0;
	}//end if
	return 1;
}
