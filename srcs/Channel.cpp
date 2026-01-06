#include "ircserv.hpp"

Channel::Channel() {}

Channel::Channel(std::string name) : Name(name) {}

Channel::~Channel() {}

std::string Channel::GetName() const { return Name; }

void Channel::AddClient(Client cli)
{
    clients.push_back(cli);
}

void Channel::RemoveClient(int fd)
{
    for (size_t i = 0; i < clients.size(); ++i)
    {
        if (clients[i].GetFd() == fd)
        {
            clients.erase(clients.begin() + i);
            break;
        }
    }
}
