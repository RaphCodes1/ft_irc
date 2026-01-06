#include "ircserv.hpp"
#include "utils.hpp"
int main()
{
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