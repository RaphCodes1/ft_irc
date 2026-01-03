#include "ircserv.hpp"

void Server::ClearClients(int fd)
{
    for(size_t i = 0; i < fds.size(); i++){
        if(fds[i].fd == fd)
            {
                fds.erase(fds.begin() + i);
                break;
            }
    }

    for(size_t i = 0; i < clients.size(); i++){
        if(clients[i].GetFd() == fd)
        {
            clients.erase(clients.begin() + i);
            break;
        }
    }
}