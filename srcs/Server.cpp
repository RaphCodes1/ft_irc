#include "ircserv.hpp"

Server::Server(int port, std::string password): Port(port), Password(password), SerSocketFd(-1){
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGPIPE, SIG_IGN);
};

Server::~Server(){
    CloseFds();
};

bool Server::Signal = false;

void Server::ClearClients(int fd)
{
    // Find the client object
    Client *cli = NULL;
    for(size_t i = 0; i < clients.size(); i++){
        if(clients[i]->GetFd() == fd) {
            cli = clients[i];
            break;
        }
    }

    if (cli) {
        // Remove client from all channels and delete empty channels
        for (size_t i = 0; i < channels.size(); i++) {
            if (channels[i]->IsClientInChannel(cli)) {
                channels[i]->RemoveClient(cli);
                if (channels[i]->GetClients().empty()) {
                    delete channels[i];
                    channels.erase(channels.begin() + i);
                    i--;
                }
            }
        }
    }

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
            delete clients[i];
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
    for(size_t i = 0; i < clients.size(); i++){
        delete clients[i];
    }
    clients.clear();

    for(size_t i = 0; i < channels.size(); i++){
        delete channels[i];
    }
    channels.clear();

    fds.clear();
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

    if(fcntl(SerSocketFd, F_SETFL, O_NONBLOCK) < 0)
        throw(std::runtime_error("fcntl failed"));

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
                    {
                        if (ReceiveNewData(fds[i].fd)) {
                            // Client disconnected and removed from fds
                            // Current 'i' now points to the next element (or invalid)
                            // Decrement i so loop increment moves to correct next element
                            i--;
                            continue;
                        }
                    }
                }
                if (i < fds.size() && (fds[i].revents & POLLOUT))
                {
                    if (HandleWrite(fds[i].fd)) {
                        i--;
                        continue;
                    }
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
        close(incofd);
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

Client* Server::GetClientByFd(int fd) {
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->GetFd() == fd)
            return clients[i];
    }
    return NULL;
}

void Server::UpdatePollOut(int fd, bool enable) {
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

void Server::QueueMessage(Client *cli, const std::string &msg) {
    if (!cli)
        return;
    cli->appendOutBuffer(msg);
    UpdatePollOut(cli->GetFd(), true);
}

bool Server::HandleWrite(int fd) {
    Client *cli = GetClientByFd(fd);
    if (!cli) {
        UpdatePollOut(fd, false);
        return false;
    }

    if (!cli->hasOutData()) {
        UpdatePollOut(fd, false);
        return false;
    }

    const std::string &out = cli->getOutBuffer();
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    ssize_t sent = send(fd, out.c_str(), out.length(), flags);
    if (sent > 0) {
        cli->consumeOutBuffer(static_cast<size_t>(sent));
        if (!cli->hasOutData())
            UpdatePollOut(fd, false);
        return false;
    }
    if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return false;

    std::cout << RED << "Client <" << fd << "> Disconnected" << RESET << std::endl;
    ClearClients(fd);
    close(fd);
    return true;
}

std::string Server::getClientHostname(int clientFd){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if(getpeername(clientFd, (struct sockaddr *)&addr, &len) < 0){
        return "unknown hostname";
    }
    return inet_ntoa(addr.sin_addr);
}

bool Server::ReceiveNewData(int fd){
    char buff[1024];
    memset(buff, 0, sizeof(buff));

    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);
    
    if(bytes <= 0)
    {
        std::cout << RED << "Client <" << fd << "> Disconnected" << RESET << std::endl;
        ClearClients(fd);
        close(fd);
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
                ParseCommand(cli, line);
                
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
