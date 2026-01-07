#ifndef SERVER_HPP
#define SERVER_HPP
#include "ircserv.hpp"

class Client;

class Server
{
    private:
        int Port;
        std::string Password;
        int SerSocketFd;
        static bool Signal;
        std::vector<Client*> clients;
        std::vector<struct pollfd> fds;
    
    public:
        Server(int port, std::string password);
        ~Server();

        void ServerInit();
        void SerSocket();
        void AcceptNewClient();
        void ReceiveNewData(int fd);
        int GetPort(){return Port;};
        std::string GetPassword(){return Password;};
        std::string getClientHostname(int clientFd);

        static void SignalHandler(int signum);

        void CloseFds();
        void ClearClients(int fd);
};

#endif