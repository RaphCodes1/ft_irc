#include "ircserv.hpp"
#include "utils.hpp"

bool validatePort(const std::string &portStr, int &port)
{   
    std::istringstream strToInt(portStr); //converting using stringstream
    int conv;

    if(!(strToInt >> conv) || !(strToInt.eof())){ //parse and validate the whole string is int
        std::cerr << RED << "Error: Invalid port number" << RESET << std::endl;
        return false;
    }

    if(conv < 1024 || conv > 65535) {
        std::cerr << RED << "Error: Port must be between 1024 and 65535" << RESET << std::endl;
        return false; 
    }

    port = conv;
    return true;
};

bool validatePass(const std::string &pass)
{   
    if(pass.empty())
    {   
        std::cerr << RED << "Password cannot be empty" << RESET << std::endl;
        return false;
    }

    if(pass.length() > 50)
    {
        std::cerr << RED << "Password cannot be longer than 50" << RESET << std::endl;
        return false;
    }

    for(size_t i = 0; i < pass.size(); i++)
    {
        if(std::isspace(static_cast<unsigned char>(pass[i])))
        {
            std::cerr << RED << "Password cannot contain whitespaces" << RESET << std::endl;
            return false; 
        }
    }
    return true;
}

int main(int ac, char **av)
{   
    if(ac == 3)
    {   
        int port;
        std::string pass = av[2];

        if(!validatePort(av[1],port))
            return -1;
        
        if(!validatePass(pass))
            return -1;
    
        //startup info
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "Starting IRC Server..." << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Password: " << pass << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "User Input: ";
        
        Server server(port, pass);
        try{
            signal(SIGINT, Server::SignalHandler);
            signal(SIGQUIT, Server::SignalHandler);
            server.ServerInit();
        }
        catch(const std::exception& e){
            server.CloseFds();
            std::cerr << e.what() << std::endl;
            return 1;
        }
        std::cout << "The Server is closed!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << RED << "Invalid Arguments!" << std::endl;
    }
}