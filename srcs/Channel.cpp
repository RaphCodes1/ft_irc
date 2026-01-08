#include "Channel.hpp"

Channel::Channel(std::string name) : Name(name) {}

Channel::~Channel() {}

void Channel::AddClient(Client *cli) {
    Clients.push_back(cli);
}

void Channel::RemoveClient(Client *cli) {
    RemoveAdmin(cli);
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

void Channel::AddAdmin(Client *cli) {
    Admins.push_back(cli);
}

void Channel::RemoveAdmin(Client *cli) {
    for (size_t i = 0; i < Admins.size(); i++) {
        if (Admins[i] == cli) {
            Admins.erase(Admins.begin() + i);
            break;
        }
    }
}

bool Channel::IsAdmin(Client *cli) {
    for (size_t i = 0; i < Admins.size(); i++) {
        if (Admins[i] == cli)
            return true;
    }
    return false;
}
