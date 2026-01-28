#include <iostream>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <csignal>
#include <cstring>
#include <sstream>
#include <cctype>
#include <cerrno>

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
        std::string outBuffer;
    public:
        Client(int fd, const std::string &hostname):Fd(fd), IPadd(hostname),Nickname(""), Username(""), Realname(""), Registered(false), LoggedIn(false){};
        ~Client(){};
        int GetFd() const {return Fd;};

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

        void appendOutBuffer(const std::string &data) { outBuffer += data; }
        void consumeOutBuffer(size_t count) { outBuffer.erase(0, count); }
        const std::string &getOutBuffer() const { return outBuffer; }
        bool hasOutData() const { return !outBuffer.empty(); }

        std::string getIpAdd()const {return this->IPadd;};
        void eraseOutBuffer(size_t n) { if (n <= outBuffer.size()) outBuffer.erase(0, n); }
};
