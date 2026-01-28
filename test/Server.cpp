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

class Server
{
    private:
        int port;
        std::string Password;
        int SerSocketFd;
        static bool signal;
        // std::vector<Client*> clients;
        // std::vector<Channel*> channel;
        std::vector<struct pollfd> fds;

    public:
        Server(int port, std::string password);
        ~Server();

        void ServerInit();

};