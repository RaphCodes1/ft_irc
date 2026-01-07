#include "ircserv.hpp"

void Server::ParseCommand(Client *cli, std::string cmd){
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
        Pass(cli, cmd);
    else if(args[0] == "NICK" || args[0] == "nick")
        Nick(cli, cmd);
    else if(args[0] == "USER" || args[0] == "user")
        User(cli, cmd);
    else if(args[0] == "JOIN" || args[0] == "join")
        Join(cli, cmd);
    else if(args[0] == "PRIVMSG" || args[0] == "privmsg")
        Privmsg(cli, cmd);
    else if(args[0] == "CAP" || args[0] == "cap")
        Cap(cli, cmd);
    else if(args[0] == "QUIT" || args[0] == "quit")
    {
        // Handle QUIT
        std::cout << "Client <" << cli->GetFd() << "> Quits" << std::endl;
        ClearClients(cli->GetFd());
        close(cli->GetFd());
    }
    else
    {
       // Unknown command, ignore for now or send ERR?
       // std::cout << "Unknown command: " << args[0] << std::endl;
    }
}

void Server::Cap(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }
    
    // CAP LS
    if (args.size() > 1 && args[1] == "LS") {
        std::string capLs = "CAP * LS :\r\n";
        send(cli->GetFd(), capLs.c_str(), capLs.length(), 0);
    }
}

void Server::Join(Client *cli, std::string cmd){
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

    std::string channelName = args[1];
    // Validate channel name (must start with #)
    if (channelName[0] != '#') {
        // ERR_NOSUCHCHANNEL or ignore? Standard says it must start with # or & etc.
        // For now simple check
        return;
    }

    Channel *channel = GetChannel(channelName);
    if (!channel) {
        channel = CreateChannel(channelName);
    }

    channel->AddClient(cli);
    
    // Notify user joined
    // :user!user@host JOIN :#channel
    std::string joinMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->GetRealname() + " JOIN :" + channelName + "\r\n";
    channel->Broadcast(joinMsg);
    // Also send to the user themselves if Broadcast excludes them? Wait, Broadcast traditionally excludes sender in some contexts but for JOIN everyone needs to know including sender (so they know they joined).
    // Let's make Broadcast send to everyone, and if I need exclude I'll pass fd.
    // My previous Broadcast implementation has an 'excludeFd' param default -1. 
    // So channel->Broadcast(joinMsg) sends to everyone. Perfect.
    
    // Send initial topic (if any) and user list (RPL_NAMREPLY etc)
    // For now minimal: just JOIN message is enough for irssi to switch window.
}

void Server::Privmsg(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 3) {
        // ERR_NEEDMOREPARAMS or ERR_NOTEXTTOSEND
        return;
    }

    std::string target = args[1];
    // The message might contain spaces, so we need everything after the target.
    // Reconstruct message from original cmd string to preserve spaces.
    size_t msgPos = cmd.find(target) + target.length();
    std::string message = cmd.substr(msgPos);
    // Trim leading spaces
    size_t firstChar = message.find_first_not_of(" ");
    if(firstChar != std::string::npos)
        message = message.substr(firstChar);
    
    // If message starts with :, remove it (IRC protocol quirk for trailing parameter)
    if (!message.empty() && message[0] == ':') {
        message = message.substr(1);
    }
    
    if (target[0] == '#') {
        // Channel message
        Channel *channel = GetChannel(target);
        if (channel) {
            // :sender!user@host PRIVMSG #channel :message
            std::string fullMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->GetRealname() + " PRIVMSG " + target + " :" + message + "\r\n";
            channel->Broadcast(fullMsg, cli->GetFd()); // Don't send back to sender
        } else {
             // ERR_NOSUCHCHANNEL
        }
    } else {
        // Private message to user
        // Find client by nickname
        Client *dest = NULL;
        for (size_t i = 0; i < clients.size(); i++) {
            if (clients[i]->GetNickname() == target) {
                dest = clients[i];
                break;
            }
        }
        if (dest) {
             std::string fullMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->GetRealname() + " PRIVMSG " + target + " :" + message + "\r\n";
             send(dest->GetFd(), fullMsg.c_str(), fullMsg.length(), 0);
        } else {
            // ERR_NOSUCHNICK
        }
    }
}

void Server::Pass(Client *cli, std::string cmd){
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

    if (CheckPassword(args[1])) {
        cli->SetLoggedIn(true);
    } else {
        // ERR_PASSWDMISMATCH
        std::string err = ":ircserv 464 " + cli->GetNickname() + " :Password incorrect\r\n";
        send(cli->GetFd(), err.c_str(), err.length(), 0);
        cli->SetLoggedIn(false);
    }
}

void Server::Nick(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 2) {
        // ERR_NONICKNAMEGIVEN
        return;
    }

    if (!cli->GetLoggedIn()) {
        // Should wait for PASS? Or if PASS not required?
        // For now assume PASS required
        return;
    }

    // Check collision...
    // Set nickname
    std::string nick = args[1];
    cli->SetNickname(nick);

    // Check if ready to register
    if (!cli->GetRegistered() && !cli->GetUsername().empty() && !cli->GetNickname().empty() && !cli->GetRealname().empty()) {
        cli->SetRegistered(true);
        // RPL_WELCOME
        std::string welcome = ":ircserv 001 " + cli->GetNickname() + " :Welcome to the ft_irc Network, " + cli->GetNickname() + "\r\n";
        send(cli->GetFd(), welcome.c_str(), welcome.length(), 0);
    }
}

void Server::User(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 5) {
        // ERR_NEEDMOREPARAMS
        return;
    }

    if (!cli->GetLoggedIn()) {
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
        cli->SetRegistered(true);
        // RPL_WELCOME
        std::string welcome = ":ircserv 001 " + cli->GetNickname() + " :Welcome to the ft_irc Network, " + cli->GetNickname() + "\r\n";
        send(cli->GetFd(), welcome.c_str(), welcome.length(), 0);
    }
}

bool Server::CheckPassword(std::string pass) {
    return pass == this->Password;
}
