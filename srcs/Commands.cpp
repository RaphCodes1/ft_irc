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
    else if(args[0] == "PART" || args[0] == "part")
        Part(cli, cmd);
    else if(args[0] == "PRIVMSG" || args[0] == "privmsg")
        Privmsg(cli, cmd);
    else if(args[0] == "CAP" || args[0] == "cap")
        Cap(cli, cmd);
    else if(args[0] == "PING" || args[0] == "ping")
        Ping(cli, cmd);
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

void Server::Ping(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }
    
    if (args.size() < 2) {
        return;
    }
    
    std::string pong = "PONG " + args[1] + "\r\n";
    send(cli->GetFd(), pong.c_str(), pong.length(), 0);
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
        channel->AddAdmin(cli);
    }

    channel->AddClient(cli);
    
    // Notify user joined
    // :user!user@host JOIN :#channel
    std::string joinMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->GetRealname() + " JOIN :" + channelName + "\r\n";
    channel->Broadcast(joinMsg);
    
    // Send RPL_NAMREPLY (353)
    // :ircserv 353 <user> = <channel> :<nick1> <nick2>
    std::string namesList = "";
    std::vector<Client*> clients = channel->GetClients();
    for (size_t i = 0; i < clients.size(); i++) {
        if (channel->IsAdmin(clients[i]))
            namesList += "@";
        namesList += clients[i]->GetNickname();
        if (i < clients.size() - 1)
            namesList += " ";
    }
    
    std::string namReply = ":ircserv 353 " + cli->GetNickname() + " = " + channelName + " :" + namesList + "\r\n";
    send(cli->GetFd(), namReply.c_str(), namReply.length(), 0);

    // Send RPL_ENDOFNAMES (366)
    std::string endNames = ":ircserv 366 " + cli->GetNickname() + " " + channelName + " :End of /NAMES list.\r\n";
    send(cli->GetFd(), endNames.c_str(), endNames.length(), 0);
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

    // Check collision
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->GetNickname() == args[1]) {
            // ERR_NICKNAMEINUSE
            std::string err = ":ircserv 433 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " " + args[1] + " :Nickname is already in use\r\n";
            send(cli->GetFd(), err.c_str(), err.length(), 0);
            return;
        }
    }

    // Set nickname
    std::string oldNick = cli->GetNickname();
    std::string newNick = args[1];
    
    if (cli->GetRegistered()) {
        // Broadcast NICK change to all channels user is in
        // :oldnick!user@host NICK :newnick
        std::string nickMsg = ":" + oldNick + "!" + cli->GetUsername() + "@" + cli->GetRealname() + " NICK :" + newNick + "\r\n";
        
        // Find channels and broadcast
        // Note: In an optimized server we would find unique users. Here we iterate channels.
        // Also send to self
        send(cli->GetFd(), nickMsg.c_str(), nickMsg.length(), 0);
        
        for (size_t i = 0; i < channels.size(); i++) {
            if (channels[i]->IsClientInChannel(cli)) {
                channels[i]->Broadcast(nickMsg, cli->GetFd()); // Don't send relative to sender again?
                // Actually my Broadcast excludes sender. But client needs to know.
                // Wait, I sent to self above. So excludeFd=cli->GetFd() is correct.
            }
        }
    }
    
    cli->SetNickname(newNick);

    // Check if ready to register
    if (!cli->GetRegistered() && !cli->GetUsername().empty() && !cli->GetNickname().empty() && !cli->GetRealname().empty()) {
        Welcome(cli);
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
    
    if (cli->GetRegistered()) {
         std::string err = ":ircserv 462 " + cli->GetNickname() + " :You may not reregister\r\n";
         send(cli->GetFd(), err.c_str(), err.length(), 0);
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
        Welcome(cli);
    }
}

bool Server::CheckPassword(std::string pass) {
    return pass == this->Password;
}

void Server::Part(Client *cli, std::string cmd) {
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token) {
        args.push_back(token);
    }
    
    if (args.size() < 2) {
        // ERR_NEEDMOREPARAMS
        return;
    }
    
    std::string channelName = args[1];
    Channel *channel = GetChannel(channelName);
    
    if (!channel) {
        // ERR_NOSUCHCHANNEL
         std::string err = ":ircserv 403 " + cli->GetNickname() + " " + channelName + " :No such channel\r\n";
         send(cli->GetFd(), err.c_str(), err.length(), 0);
        return;
    }
    
    if (!channel->IsClientInChannel(cli)) {
        // ERR_NOTONCHANNEL
        std::string err = ":ircserv 442 " + cli->GetNickname() + " " + channelName + " :You're not on that channel\r\n";
        send(cli->GetFd(), err.c_str(), err.length(), 0);
        return;
    }
    
    // Broadcast PART message to channel (including user)
    // :user!user@host PART #channel
    std::string partMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->GetRealname() + " PART " + channelName + "\r\n";
    channel->Broadcast(partMsg);
    
    channel->RemoveClient(cli);
    
    // If channel is empty, delete it (optional but good practice)
    if (channel->GetClients().empty()) {
        // Remove from channels vector
        for (size_t i = 0; i < channels.size(); i++) {
            if (channels[i] == channel) {
                channels.erase(channels.begin() + i);
                delete channel;
                break;
            }
        }
    }
}

void Server::Welcome(Client *cli) {
    cli->SetRegistered(true);
    // RPL_WELCOME
    std::string welcome = ":ircserv 001 " + cli->GetNickname() + " :Welcome to the ft_irc Network, " + cli->GetNickname() + "\r\n";
    send(cli->GetFd(), welcome.c_str(), welcome.length(), 0);

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
    send(cli->GetFd(), help.c_str(), help.length(), 0);
}
