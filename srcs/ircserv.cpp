#include "ircserv.hpp"

int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return 1;
	}
	Server ser;
	std::cout << "---- SERVER ----" << std::endl;
	try
	{
		ser.ServerInit(std::atoi(av[1]), av[2]);
	}
	catch(const std::exception& e)
	{
		ser.CloseFds();
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
	return 0;
}