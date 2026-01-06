#include "ircserv.hpp"

bool Server::Signal = false;

void Server::SignalHandler(int signum)
{
	(void)signum;
	std::cout << std::endl << "Signal Received!" << std::endl;
	Signal = true;
}

void Server::CloseFds()
{
	for(size_t i = 0; i < clients.size(); i++)
	{
		std::cout << RED << "Client <" << clients[i].GetFd() << "> Disconnected" << WHI << std::endl;
		close(clients[i].GetFd());
	}
	if (SerSocketFd != -1)
	{
		std::cout << RED << "Server <" << SerSocketFd << "> Disconnected" << WHI << std::endl;
		close(SerSocketFd);
	}
}

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
		if(clients[i].GetFd() == fd)
		{
			clients.erase(clients.begin() + i);
			break;
		}
	}
}

void Server::ServerInit(int port, std::string pass)
{
	Port = port;
	Password = pass;
	SerSocket();

	std::cout << GRE << "Server <" << SerSocketFd << "> Connected" << WHI << std::endl;
	std::cout << "Waiting to accept a connection..." << std::endl;

	while (Server::Signal == false)
	{
		if((poll(&fds[0], fds.size(), -1) == -1) && Server::Signal == false)
			throw(std::runtime_error("poll() failed"));

		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == SerSocketFd)
					AcceptNewClient();
				else
					RecieveNewData(fds[i].fd);
			}
		}
	}
	CloseFds();
}

void Server::SerSocket()
{
	struct sockaddr_in add;
	struct pollfd NewPoll;
	add.sin_family = AF_INET;
	add.sin_port = htons(this->Port);
	add.sin_addr.s_addr = INADDR_ANY;

	SerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if(SerSocketFd == -1)
		throw(std::runtime_error("failed to create socket"));

	int en = 1;
	if (setsockopt(SerSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
		throw(std::runtime_error("failed to set option (SO_REUSEADDR) on socket"));
	if (fcntl(SerSocketFd, F_SETFL, O_NONBLOCK) == -1)
		throw(std::runtime_error("failed to set option (O_NONBLOCK) on socket"));
	if (bind(SerSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1)
		throw(std::runtime_error("failed to bind socket"));
	if (listen(SerSocketFd, SOMAXCONN) == -1)
		throw(std::runtime_error("listen() failed"));

	NewPoll.fd = SerSocketFd;
	NewPoll.events = POLLIN;
	NewPoll.revents = 0;
	fds.push_back(NewPoll);
}

void Server::AcceptNewClient()
{
	Client cli;
	struct sockaddr_in cliadd;
	struct pollfd NewPoll;
	socklen_t len = sizeof(cliadd);

	int incofd = accept(SerSocketFd, (struct sockaddr *)&cliadd, &len);
	if (incofd == -1)
	{
		std::cout << "accept() failed" << std::endl;
		return;
	}

	if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cout << "fcntl() failed" << std::endl;
		return;
	}

	NewPoll.fd = incofd;
	NewPoll.events = POLLIN;
	NewPoll.revents = 0;

	cli.setFd(incofd);
	cli.setIpAdd(inet_ntoa(cliadd.sin_addr));
	clients.push_back(cli);
	fds.push_back(NewPoll);

	std::cout << GRE << "Client <" << incofd << "> Connected" << WHI << std::endl;
}

void Server::RecieveNewData(int fd)
{
	char buff[1024];
	memset(buff, 0, sizeof(buff));

	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1 , 0);

	if(bytes <= 0)
	{
		std::cout << RED << "Client <" << fd << "> Disconnected" << WHI << std::endl;
		ClearClients(fd);
		close(fd);
	}
	else
	{
		buff[bytes] = '\0';
        // helper to find client
        Client *cli = NULL;
        for(size_t i = 0; i < clients.size(); i++)
        {
            if(clients[i].GetFd() == fd)
            {
                cli = &clients[i];
                break;
            }
        }
        if(!cli) return;

        cli->AddBuffer(buff);
        std::string buffer = cli->GetBuffer();
        
        size_t pos = 0;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string cmd = buffer.substr(0, pos);
            if (!cmd.empty() && cmd[cmd.length() - 1] == '\r')
                cmd.erase(cmd.length() - 1);
            
            Parse(cli, cmd);
            buffer.erase(0, pos + 1);
        }
        
        cli->ClearBuffer();
        cli->AddBuffer(buffer);
	}
}