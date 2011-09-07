#include"Interface.h"

Interface::Interface() : m_port(80)
{
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
}
