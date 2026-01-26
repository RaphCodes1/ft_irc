#include "Channel.hpp"
#include "Server.hpp"

Channel::Channel(std::string name) : Name(name), Limit(0), ModeI(false), ModeT(true), ModeK(false), ModeL(false) {}

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

void Channel::Broadcast(Server *server, std::string msg, int excludeFd) {
    if (!server)
        return;
    for (size_t i = 0; i < Clients.size(); ++i) {
        if (Clients[i]->GetFd() != excludeFd) {
            server->QueueMessage(Clients[i], msg);
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

std::string Channel::GetModeString() {
    std::string mode = "+";
    if(ModeI) mode += "i";
    if(ModeT) mode += "t";
    if(ModeK) mode += "k";
    if(ModeL) mode += "l";
    return mode;
}

void Channel::AddInvited(std::string nick) {
    InvitedNicks[nick] = std::time(NULL);
}

bool Channel::IsInvited(std::string nick) {
    std::map<std::string, time_t>::iterator it = InvitedNicks.find(nick);
    if (it != InvitedNicks.end()) {
        if (std::difftime(std::time(NULL), it->second) < 60) {
            return true;
        } else {
            InvitedNicks.erase(it);
        }
    }
    return false;
}

void Channel::RemoveInvited(std::string nick) {
    InvitedNicks.erase(nick);
}
