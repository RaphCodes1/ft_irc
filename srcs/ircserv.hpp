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

class Client
{
    private:
        int Fd;
        std::string IPadd;
    public:
        Client(){};
        ~Client(){};
        int GetFd(){return Fd;};

        void setFd(int fd){Fd = fd;};
        void setIpAdd(std::string ipadd){IPadd = ipadd;};
};

class Server
{
    private:
        int Port;
        int SerSocketFd;
        static bool Signal;
        std::vector<Client> clients;
        std::vector<struct pollfd> fds;
    
    public:
        Server(){SerSocketFd = -1;};
        ~Server(){};

        void ServerInit();
        void SerSocket();
        void AcceptNewClient();
        void RecieveNewData(int fd);

        static void SignalHandler(int signum);

        void CloseFds();
        void ClearClients(int fd);
};