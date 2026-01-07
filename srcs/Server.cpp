#include "ircserv.hpp"

Server::Server(int port, std::string password): Port(port), Password(password), SerSocketFd(-1){
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGSEGV, SignalHandler);
};

Server::~Server(){
    CloseFds();
};

bool Server::Signal = false;

void Server::ClearClients(int fd)
{
    for(size_t i = 0; i < fds.size(); i++){
        if(fds[i].fd == fd)
            {
                fds.erase(fds.begin() + i);
                break;
            }
    }

    for(size_t i = 0; i < clients.size(); i++){
        if(clients[i]->GetFd() == fd)
        {
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

void Server::SignalHandler(int signum)
{
    (void)signum;
    std::cout << std::endl << "Signal is recieved!" << std::endl;
    Server::Signal = true;
}

void Server::CloseFds(){
    for(size_t i = 0; i < clients.size();i++){
        std::cout << RED << "Client " << clients[i]->GetFd() << " Disconnected" << RESET << std::endl;
        close(clients[i]->GetFd());
    }
    if(SerSocketFd != -1){
        std::cout << RED << "Server <" << SerSocketFd << "> Disconnected" << RESET << std::endl;
        close(SerSocketFd);
    }
}

void Server::SerSocket(){
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // any adress
    addr.sin_port = htons(this->Port); // convert the port to network byte order(big endian)
    
    SerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if(SerSocketFd < 0){
        throw(std::runtime_error("failed to create socket"));
    }
    
    int opt = 1;
    if(setsockopt(SerSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // set socket option (SO_REUSEADDR) to reuse address
        throw(std::runtime_error("setsockopt failed"));

    if(bind(SerSocketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) // bind the socket to the address
        throw(std::runtime_error("bind failed"));

    if(listen(SerSocketFd, SOMAXCONN) < 0) // listen for connections
        throw(std::runtime_error("listen failed"));

    struct pollfd NewPollFd;
    NewPollFd.fd = SerSocketFd; //  add the server pocket to the pollfd
    NewPollFd.events = POLLIN; // set the event to POLLIN for reading data
    NewPollFd.revents = 0; // set the revents to 0
    fds.push_back(NewPollFd); // add the server to the pollfd
}

void Server::ServerInit()
{   
    SerSocket();

    std::cout << GREEN << "Server started on port " << this->Port << RESET << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    try{
        while(Server::Signal == false){
            if((poll(&fds[0],fds.size(), -1) == -1) || (Server::Signal == true))
            throw(std::runtime_error("shutting down..."));
            
            for(size_t i = 0; i < fds.size(); i++){
                if(fds[i].revents & POLLIN)
                {
                    if(fds[i].fd == SerSocketFd)
                        AcceptNewClient();
                    else
                        ReceiveNewData(fds[i].fd);
                }
            }
        }
    }
    catch(const std::exception& e){
        std::cerr << "Server Error: " << e.what() << std::endl;
    }
    CloseFds();
}

void Server::AcceptNewClient(){
    struct sockaddr_in addr;
    struct pollfd NewPoll;
    socklen_t len = sizeof(addr);
    
    int incofd = accept(SerSocketFd, (struct sockaddr *)&addr, &len);
    if(incofd < 0)
    {
        std::cerr << "Accept failed" << std::endl;
        return;
    }
    
    if(fcntl(incofd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "fcntl() failed" << std::endl;
        return;
    }
    
    Client* cli = new Client(incofd, getClientHostname(incofd));
    clients.push_back(cli);

    NewPoll.fd = incofd;
    NewPoll.events = POLLIN;
    NewPoll.revents = 0;
    fds.push_back(NewPoll);

    std::cout << GREEN << "Client <" << incofd << "> Connected" << RESET << std::endl;
}

std::string Server::getClientHostname(int clientFd){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if(getpeername(clientFd, (struct sockaddr *)&addr, &len) < 0){
        return "unknown hostname";
    }
    return inet_ntoa(addr.sin_addr);
}

void Server::ReceiveNewData(int fd){
    char buff[1024];
    memset(buff, 0, sizeof(buff));

    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);
    
    if(bytes <= 0)
    {
        std::cout << RED << "Client <" << fd << "> Disconnected" << RESET << std::endl;
        ClearClients(fd);
        close(fd);
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
                ParseCommand(cli, line);
                tmp.erase(0, pos + 1);
            }
            cli->setBuffer(tmp);
        }
    }
}

Channel* Server::GetChannel(std::string name){
    for (size_t i = 0; i < channels.size(); i++){
        if(channels[i]->GetName() == name)
            return channels[i];
    }
    return NULL;
}

Channel* Server::CreateChannel(std::string name){
    Channel *newChannel = new Channel(name);
    channels.push_back(newChannel);
    return newChannel;
}
