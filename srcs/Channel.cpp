#include "Channel.hpp"

Channel::Channel(std::string name) : Name(name) {}

Channel::~Channel() {}

void Channel::AddClient(Client *cli) {
    Clients.push_back(cli);
}

void Channel::RemoveClient(Client *cli) {
    for (size_t i = 0; i < Clients.size(); ++i) {
        if (Clients[i]->GetFd() == cli->GetFd()) {
            Clients.erase(Clients.begin() + i);
            break;
        }
    }
}

bool Channel::IsClientInChannel(Client *cli) {
    for (size_t i = 0; i < Clients.size(); ++i) {
        if (Clients[i]->GetFd() == cli->GetFd()) {
            return true;
        }
    }
    return false;
}

void Channel::Broadcast(std::string msg, int excludeFd) {
    for (size_t i = 0; i < Clients.size(); ++i) {
        if (Clients[i]->GetFd() != excludeFd) {
            send(Clients[i]->GetFd(), msg.c_str(), msg.length(), 0);
        }
    }
}
