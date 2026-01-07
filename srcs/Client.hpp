#ifndef CLIENT_HPP
#define CLIENT_HPP
#include "ircserv.hpp"

class Client
{
    private:
        int Fd;
        std::string IPadd;
        std::string Nickname;
        std::string Username;
        std::string Realname;
        bool Registered;
        bool LoggedIn;
        std::string buffer;
    public:
        Client(int fd, const std::string &hostname);
        ~Client();
        int GetFd()const {return Fd;};

        void setFd(int fd){Fd = fd;};
        void setIpAdd(std::string ipadd){IPadd = ipadd;};

        void SetNickname(std::string& nickName) { this->Nickname = nickName; }
        void SetUsername(std::string& userName) { this->Username = userName; }
        void SetRealname(std::string& realName) { this->Realname = realName; }
        void SetLoggedIn(bool value) { this->LoggedIn = value; }
        void SetRegistered(bool value) { this->Registered = value; }

        std::string GetNickname()const { return this->Nickname; }
        std::string GetUsername()const { return this->Username; }
        std::string GetRealname()const { return this->Realname; }
        bool GetLoggedIn()const { return this->LoggedIn; }
        bool GetRegistered()const { return this->Registered; }

        void setBuffer(std::string buf) { buffer = buf; }
        std::string getBuffer()const { return buffer; }
        void clearBuffer() { buffer.clear(); }
};

#endif