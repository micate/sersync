#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
//#include<curl/curl.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include "QueueFilter.h"
#include "Initialize.h"
#include "xmlParser.h"
#include "Inotify.h"
#include "SocketInterface.h"

SocketInterface::SocketInterface() : m_ip("127.0.0.1")
{

}

void SocketInterface::XmlParse(std::string config_file_name)
{
    XMLNode xMainNode = XMLNode::openFileHelper(config_file_name.c_str(), "head");
    int num = xMainNode.nChildNode("plugin");
    if (num == 0)
    {
        perror("Error there is no plugin tag in config xml\n");
        exit(1);
    }
    for (int i = 0; i < num; i++)
    {
        string name = xMainNode.getChildNode("plugin", i).getAttribute("name");
        if (name == "socket")
        {
            XMLNode xNode = xMainNode.getChildNode("plugin", i);
            m_watch = xNode.getChildNode("localpath").getAttribute("watch");
            m_ip = xNode.getChildNode("localpath").getChildNode("deshost").getAttribute("ip");
            m_port = atoi(xNode.getChildNode("localpath").getChildNode("deshost").getAttribute("port"));
            cout << "监控路径: " << m_watch << "  ip:  " << m_ip << " 端口号: " << m_port << endl;
            break;
        }
    }
}

int SocketInterface::Execute(Event e)
{
    static int i = 1;
    int sockfd = 0;
    int numbytes = 0;
    int flags = 0;
    int cycletimes = 0;
    char sendBuffer[2048] = "\0";
    fd_set wset;
    sprintf(sendBuffer, "%s", e->path.c_str());

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_port);
    serv_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    bzero(&(serv_addr.sin_zero), 8);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        close(sockfd);
        return 2;
    }

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); //设置为非阻塞;
    do
    {
        connect(sockfd, (struct sockaddr *) & serv_addr, sizeof (struct sockaddr));
        FD_ZERO(&wset);
        FD_SET(sockfd, &wset);
        if (select(sockfd + 1, NULL, &wset, NULL, &timeout) <= 0 && cycletimes == 5)
        {
            close(sockfd);
            return 5;
        }
        numbytes = send(sockfd, sendBuffer, strlen(sendBuffer), MSG_NOSIGNAL);
        if (numbytes < 0)
        {
            usleep(20000);
        }
        cycletimes++;
    } while (numbytes < 0 && cycletimes != 5);
    if (numbytes < 0)
    {
        close(sockfd);
        return 4;
    }
    close(sockfd);
    return 0;
}
