#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <map>

// Colors for output
#define RED "\e[1;31m"
#define WHI "\e[0;37m"
#define GRE "\e[1;32m"
#define YEL "\e[1;33m"



class Client
{
    private:
        int Fd;
        std::string IPadd;
        std::string buffer;
        std::string Nickname;
        std::string Username;
        std::string Realname;
        bool Registered;
        bool LoggedIn;

    public:
        Client();
        Client(int fd, std::string ip);
        ~Client();
        
        int GetFd() const;
        std::string GetIp() const;
        std::string GetNick() const;
        std::string GetUser() const;
        std::string GetReal() const;
        std::string GetBuffer() const;
        bool GetRegistered() const;
        bool GetLoggedIn() const;

        void setFd(int fd);
        void setIpAdd(std::string ipadd);
        void setNick(std::string nick);
        void setUser(std::string user);
        void setReal(std::string real);
        void setRegistered(bool val);
        void setLoggedIn(bool val);
        
        void AddBuffer(std::string data);
        void ClearBuffer();
};

class Channel
{
    private:
        std::string Name;
        // std::string Topic;
        // std::string Key;
        std::vector<Client> clients;
        std::vector<Client> admins;

        // bool mode_i;
        // bool mode_t;
        // bool mode_k;
        // bool mode_l;
        // size_t limit;

    public:
        Channel();
        Channel(std::string name);
        ~Channel();


        std::string GetName() const;
        void AddClient(Client cli);
        void RemoveClient(int fd); // or by reference
        std::vector<Client> &GetClients() {return clients;};
};



class Server
{
    private:
        int Port;
        std::string Password;
        int SerSocketFd;
        static bool Signal;
        std::vector<Client> clients;
        std::vector<struct pollfd> fds;
        std::vector<Channel> channels;
    
    public:
        Server(){SerSocketFd = -1;};
        ~Server(){};

        void ServerInit(int port, std::string pass);
        void SerSocket();
        void AcceptNewClient();
        void RecieveNewData(int fd);

        static void SignalHandler(int signum);

        void CloseFds();
        void ClearClients(int fd);
        
        void Parse(Client *cli, std::string cmd);

};


#endif