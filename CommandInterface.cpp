#include <string.h>
#include "QueueFilter.h"
#include "Initialize.h"
#include "xmlParser.h"
#include "CommandInterface.h"

CommandInterface::CommandInterface() : m_prefix("/bin/sh"), m_ignoreError(false), m_debug(0), m_filter(false)
{

}

void CommandInterface::XmlParse(std::string config_file_name)
{
    XMLNode xMainNode = XMLNode::openFileHelper(config_file_name.c_str(), "head");
    string tmp = xMainNode.getChildNode("debug").getAttribute("start");
    if (tmp == "true")	m_debug = 1;
    XMLNode Node = xMainNode.getChildNode("sersync");
    m_watch = Node.getChildNode("localpath").getAttribute("watch");   
    int num = xMainNode.nChildNode("plugin");
    if (num == 0)
    {
        perror("Error there is no plugin tag in config xml\n");
        exit(1);
    }
    for (int i = 0; i < num; i++)
    {
        string name = xMainNode.getChildNode("plugin", i).getAttribute("name");
        if (name == "command")
        {
            XMLNode xNode = xMainNode.getChildNode("plugin", i);
            m_prefix = xNode.getChildNode("param").getAttribute("prefix");
            m_suffix = xNode.getChildNode("param").getAttribute("suffix");
            string tmp = xNode.getChildNode("param").getAttribute("ignoreError");
            if (tmp == "true")
            {
                m_ignoreError = true;
            }
            tmp = xNode.getChildNode("filter").getAttribute("start");
            if (tmp == "true")
            {
                m_filter = true;
                int num = xNode.getChildNode("filter").nChildNode("include");
                for (int i = 0; i < num; i++)
                {
                    string t = xNode.getChildNode("filter").getChildNode(i).getAttribute("expression");
                    try
                    {
                        ptrRegex tmp(new boost::regex(t));
                        pattern.push_back(tmp);

                    } catch (boost::regex_error& e)
                    {
                        cout << "plugin command regular expression error" << endl;
                        cout << "error information is:\t" << e.what() << endl;
                        exit(1);
                    }
                }
            }
            break;
        }
    }

}

int CommandInterface::Execute(Event e)
{
    //if (e->operation == 0 || e->mask==256) return 0;
    boost::cmatch what;
    if (m_filter == true)
    {
        int valid = 0;
        for (int i = 0; i < pattern.size(); i++)
        {
            if (boost::regex_match((e->path).c_str(), what, *(pattern[i])))
            {
                //cout<<"this is other"<<what[1]<<endl;
                valid = 1;
                break;
            }
        }
        if (!valid) return 0;
    }
    string command = m_prefix + " " + e->path + " " + m_suffix;
    if (m_ignoreError == true)
    {
        command += " >/dev/null 2>&1 ";
    }
    if (m_debug == 1)
    {
        cout << command << endl;
    }
    system(command.c_str());
}
