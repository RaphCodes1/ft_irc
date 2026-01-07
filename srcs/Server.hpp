#ifndef SERVER_HPP
#define SERVER_HPP
#include "ircserv.hpp"
#include "Channel.hpp"

class Client;
class Channel;

class Server
{
    private:
        int Port;
        std::string Password;
        int SerSocketFd;
        static bool Signal;
        std::vector<Client*> clients;
        std::vector<Channel*> channels;
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

        void Nick(Client *cli, std::string cmd);
        void User(Client *cli, std::string cmd);
        void Pass(Client *cli, std::string cmd);
        void Join(Client *cli, std::string cmd);
        void Privmsg(Client *cli, std::string cmd);
        void Cap(Client *cli, std::string cmd);
        void ParseCommand(Client *cli, std::string cmd);
        
        bool CheckPassword(std::string pass);

        Channel* GetChannel(std::string name);
        Channel* CreateChannel(std::string name);

        static void SignalHandler(int signum);

        void CloseFds();
        void ClearClients(int fd);
};

#endif