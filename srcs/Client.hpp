#ifndef CLIENT_HPP
#define CLIENT_HPP
#include "ircserv.hpp"

class Client
{
    private:
        int Fd;
        std::string IPadd;
    public:
        Client(int fd, const std::string &hostname);
        ~Client();
        int GetFd()const {return Fd;};

        void setFd(int fd){Fd = fd;};
        void setIpAdd(std::string ipadd){IPadd = ipadd;};
};

#endif