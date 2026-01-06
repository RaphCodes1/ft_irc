#include "ircserv.hpp"
#include "utils.hpp"
int main(int ac, char **av)
{   
    if(ac == 3)
    {   
        std::cout << "User Input: ";
        for(size_t i = 1; i < 3; i++)
            std::cout << av[i] << " ";
        std::cout << std::endl;

        Server server;
        std::cout << "----SERVER----" << std::endl;
        try{
            signal(SIGINT, Server::SignalHandler);
            signal(SIGQUIT, Server::SignalHandler);
            server.ServerInit();
        }
        catch(const std::exception& e){
            server.CloseFds();
            std::cerr << e.what() << std::endl;
            return 1;
        }
        std::cout << "The Server is closed!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << RED << "Invalid Arguments!" << std::endl;
    }
}