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
#include <utility>


#include "Client.hpp"

struct pollfd SerSocket(int port, struct sockaddr_in addr, int SerSocketFd)
{
    (void)port;
    int opt = 1;
    // ... (rest of function)

    if(setsockopt(SerSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw(std::runtime_error("setsockopt failed"));

    if(bind(SerSocketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) // bind the socket to the address
    throw(std::runtime_error("bind failed"));

    if(listen(SerSocketFd, SOMAXCONN) < 0) // listen for connections
        throw(std::runtime_error("listen failed"));

    if(fcntl(SerSocketFd, F_SETFL, O_NONBLOCK) < 0)
        throw(std::runtime_error("fcntl failed"));

    struct pollfd NewPoll;
    NewPoll.fd = SerSocketFd;
    NewPoll.events = POLLIN;
    NewPoll.revents = 0;
    return NewPoll;
}

bool Signal = false;

void SignalHandler(int signum)
{
    (void)signum;
    std::cout << std::endl << "Signal is recieved!" << std::endl;
    Signal = true;
}

std::string getClientHostname(int clientFd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if(getpeername(clientFd, (struct sockaddr *)&addr, &len) < 0){
        return "unknown hostname";
    }
    return inet_ntoa(addr.sin_addr);
}

std::pair<int, std::string> AcceptNewClient(int SerSocketFd)
{
    struct sockaddr_in addr;
    // struct pollfd NewPoll; // Unused
    socklen_t len = sizeof(addr);
    std::pair<int, std::string> pair(-1, "");

    int incofd = accept(SerSocketFd, (struct sockaddr *)&addr, &len);
    if(incofd < 0)
    {
        std::cerr << "Accept failed" << std::endl;
        return pair;
    }

    if(fcntl(incofd,F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "fcntl() failed" << std::endl;
        close(incofd);
        return pair;
    }

    std::pair<int, std::string> finalPair(incofd, getClientHostname(incofd));
    return finalPair;
}

bool CheckPassword(std::string pass, std::string ServPass) {
    return pass == ServPass;
}

void UpdatePollOut(int fd, bool enable, std::vector<struct pollfd> &fds) {
    for (size_t i = 0; i < fds.size(); i++) {
        if (fds[i].fd == fd) {
            if (enable)
                fds[i].events = POLLIN | POLLOUT;
            else
                fds[i].events = POLLIN;
            break;
        }
    }
}

void QueueMessage(Client *cli, const std::string &msg, std::vector<struct pollfd> &fds) {
    if (!cli)
        return;
    cli->appendOutBuffer(msg);
    UpdatePollOut(cli->GetFd(), true, fds);
}

void Welcome(Client *cli, std::vector<struct pollfd> &fds) {
    cli->SetRegistered(true);
    // RPL_WELCOME
    std::string welcome = ":ircserv 001 " + cli->GetNickname() + " :Welcome to the ft_irc Network, " + cli->GetNickname() + "\r\n";
    QueueMessage(cli, welcome, fds);

    // Send Help/Available Commands
    std::string help = ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :Available Commands:\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :JOIN #channel - Join a channel\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :PART #channel - Leave a channel\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :PRIVMSG #channel :message - Send message to channel\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :PRIVMSG nickname :message - Send private message\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :TOPIC #channel :topic - Set channel topic (ops only)\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :KICK #channel nickname - Kick user (ops only)\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :INVITE nickname #channel - Invite user (ops only)\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :MODE #channel +/-itklno - Set channel modes (ops only)\r\n"
                       ":ft_irc.42.fr NOTICE " + cli->GetNickname() + " :QUIT - Disconnect from server\r\n";
    QueueMessage(cli, help, fds);
}

void Pass(Client *cli, std::string cmd, std::string pass, std::vector<struct pollfd> &fds){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 2) {
        // ERR_NEEDMOREPARAMS
        return;
    }

    if (CheckPassword(args[1], pass)) {
        cli->SetLoggedIn(true);
    } else {
        // ERR_PASSWDMISMATCH
        std::string err = ":ircserv 464 " + cli->GetNickname() + " :Password incorrect\r\n";
        QueueMessage(cli, err, fds);
        cli->SetLoggedIn(false);
    }
}

void Nick(Client *cli, std::string cmd, std::vector<struct pollfd> &fds){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 2) {
        // ERR_NONICKNAMEGIVEN (431)
        std::string err = ":ircserv 431 * :No nickname given\r\n";
        QueueMessage(cli, err, fds);
        return;
    }

    if (!cli->GetLoggedIn()) {
        // ERR_NOTREGISTERED (451) - must send PASS first
        std::string err = ":ircserv 451 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " NICK :You have not registered\r\n";
        QueueMessage(cli, err, fds);
        return;
    }

    // Check collision
    // for (size_t i = 0; i < clients.size(); i++) {
    //     if (clients[i]->GetNickname() == args[1]) {
    //         // ERR_NICKNAMEINUSE
    //         std::string err = ":ircserv 433 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " " + args[1] + " :Nickname is already in use\r\n";
    //         QueueMessage(cli, err);
    //         return;
    //     }
    // }

    // Set nickname
    std::string oldNick = cli->GetNickname();
    std::string newNick = args[1];
     
    cli->SetNickname(newNick);

    // Check if ready to register
    if (!cli->GetRegistered() && !cli->GetUsername().empty() && !cli->GetNickname().empty() && !cli->GetRealname().empty()) {
        Welcome(cli, fds);
    }
}

void User(Client *cli, std::string cmd, std::vector<struct pollfd> &fds){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 5) {
        // ERR_NEEDMOREPARAMS (461)
        std::string err = ":ircserv 461 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " USER :Not enough parameters\r\n";
        QueueMessage(cli, err, fds);
        return;
    }

    if (!cli->GetLoggedIn()) {
        // ERR_NOTREGISTERED (451)
        std::string err = ":ircserv 451 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " USER :You have not registered\r\n";
        QueueMessage(cli, err, fds);
        return;
    }
    
    if (cli->GetRegistered()) {
         std::string err = ":ircserv 462 " + cli->GetNickname() + " :You may not reregister\r\n";
            QueueMessage(cli, err, fds);
         return;
    }

    std::string username = args[1];
    // realname might contain spaces and is the last arg, starts with : usually.
    // For now simple parsing:
    std::string realname = cmd.substr(cmd.find(":") + 1); 

    cli->SetUsername(username);
    cli->SetRealname(realname);

    // Check if ready to register
    if (!cli->GetRegistered() && !cli->GetUsername().empty() && !cli->GetNickname().empty() && !cli->GetRealname().empty()) {
        Welcome(cli, fds);
    }
}



void ParseCommand(Client *cli, std::string cmd, std::string pass, std::vector<struct pollfd> &fds)
{
    if(cmd.empty())
        return;
    
    // Split command and arguments
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if(args.empty())
        return;

    if(args[0] == "PASS" || args[0] == "pass")
        Pass(cli, cmd, pass, fds);
    else if(args[0] == "NICK" || args[0] == "nick")
        Nick(cli, cmd, fds);
    else if(args[0] == "USER" || args[0] == "user")
        User(cli, cmd, fds);
    else if(args[0] == "CAP" || args[0] == "cap") {
        if (args.size() > 1 && args[1] == "LS") {
             std::string cap_reply = "CAP * LS :\r\n";
             QueueMessage(cli, cap_reply, fds);
        }
        return; 
    }
    else if(args[0] == "JOIN" || args[0] == "join") {
        // Irssi bogus JOIN : test - send error (461 insufficient params)
        std::string err = ":ircserv 461 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " JOIN :Not enough parameters\r\n";
        QueueMessage(cli, err, fds);
        return;
    }
    else
    {
        
    }
}

bool RecieveNewData(int fd, std::vector<Client*> &clients, std::vector<struct pollfd> &fds, std::string pass)
{
    char buff[1024];
    memset(buff, 0, sizeof(buff));

    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);
    
    if(bytes <= 0)  // Add stillConnected check here too
    {
        std::cout << "Client <" << fd << "> Disconnected" << std::endl;
        
        // CLEANUP: Remove from clients
        for(size_t j = 0; j < clients.size(); j++) {
            if(clients[j]->GetFd() == fd) {
                delete clients[j];
                clients.erase(clients.begin() + j);
                break;
            }
        }
        
        // CLEANUP: Remove from fds (backwards to avoid index shift)
        for(int j = fds.size() - 1; j >= 0; j--) {
            if(fds[j].fd == fd) {
                close(fd);
                fds.erase(fds.begin() + j);
                break;
            }
        }
        return true;
    }
    else
    { 
        buff[bytes] = '\0';
        // std::cout << "Client <" << fd << ">: " << buff << std::endl;
        // ParseCommand(fd, buff);
        // Find client
        Client *cli = NULL;
        for(size_t i = 0; i < clients.size(); i++){
            if(clients[i]->GetFd() == fd){
                cli = clients[i];
                break;
            }
        }
        if (cli) {
            cli->setBuffer(cli->getBuffer() + buff);
            std::string tmp = cli->getBuffer();
            size_t pos = 0;
            while ((pos = tmp.find("\n")) != std::string::npos) {
                std::string line = tmp.substr(0, pos);
                if (line.length() > 0 && line[line.length() - 1] == '\r')
                    line = line.substr(0, line.length() - 1);
                
                std::cout << "Client <" << cli->GetFd() << ">: " << line << std::endl;
                ParseCommand(cli, line, pass, fds);
                
                // Check if client was disconnected (QUIT)
                bool stillConnected = false;
                for(size_t i = 0; i < fds.size(); i++) {
                    if(fds[i].fd == fd) {
                        stillConnected = true;
                        break;
                    }
                }
                if (!stillConnected) return true;

                tmp.erase(0, pos + 1);
            }
            cli->setBuffer(tmp);
        }
    }
    return false;
}
int main(int ac, char **av)
{
    if(ac == 3)
    {
        int port;
        std::string pass = av[2];
        std::vector<struct pollfd> fds;
        std::vector<Client*> clients;

        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "Starting the test IRC server" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;

        std::istringstream conv(av[1]);
        conv >> port;

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        int SerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if(SerSocketFd < 0){
            throw(std::runtime_error("failed to create a socket"));
        }
        
        fds.push_back(SerSocket(port, addr, SerSocketFd));
        try{
            signal(SIGINT, SignalHandler);
            signal(SIGTERM, SignalHandler);
            
            while(Signal != true){
                int poll_ret = poll(&fds[0], fds.size(), 1000);  // 1s timeout
                if(poll_ret == -1) {
                    std::cerr << "poll() failed: " << strerror(errno) << std::endl;
                    if(errno == EINTR) continue;  // Signal interrupt OK
                    break;  // Don't throw, graceful exit
                }
                if(poll_ret == 0) continue;  // Timeout OK
                
                for(size_t i = 0; i < fds.size(); i++)
                {
                    if(fds[i].revents & POLLIN)
                    {
                        if(fds[i].fd == SerSocketFd)
                        {
                            std::pair<int, std::string> pair = AcceptNewClient(SerSocketFd);
                            Client *cli = new Client(pair.first, pair.second);
                            clients.push_back(cli);

                            struct pollfd NewPoll;

                            NewPoll.fd = pair.first;
                            NewPoll.events = POLLIN;
                            NewPoll.revents = 0;
                            fds.push_back(NewPoll);
                        }
                        else
                        {
                            if(RecieveNewData(fds[i].fd, clients, fds, pass))
                            {
                                --i;
                                continue;
                            }
                        }
                    }
                    else if (fds[i].revents & POLLOUT)
                    {
                        Client *cli = NULL;
                        for(size_t j = 0; j < clients.size(); j++) {  // Inline find_client
                            if(clients[j]->GetFd() == fds[i].fd) {
                                cli = clients[j];
                                break;
                            }
                        }
                        if (cli && !cli->getOutBuffer().empty()) {
                            ssize_t sent = send(fds[i].fd, cli->getOutBuffer().c_str(), 
                                                cli->getOutBuffer().size(), 0);
                            if (sent > 0) 
                                cli->eraseOutBuffer(sent);  // Add this Client method: erase first N chars
                            if (cli->getOutBuffer().empty()) 
                                UpdatePollOut(fds[i].fd, false, fds);
                        }
                    }
                }
            }
        }catch(const std::exception &e){
            std::cerr << "Server Error: " << e.what() << std::endl;
        }
    }
}