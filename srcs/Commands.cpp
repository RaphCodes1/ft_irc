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
    else if(args[0] == "KICK" || args[0] == "kick")
        Kick(cli, cmd);
    else if(args[0] == "INVITE" || args[0] == "invite")
        Invite(cli, cmd);
    else if(args[0] == "TOPIC" || args[0] == "topic")
        Topic(cli, cmd);
    else if(args[0] == "MODE" || args[0] == "mode")
        Mode(cli, cmd);
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
        QueueMessage(cli, capLs);
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
        // ERR_NEEDMOREPARAMS (409)
        std::string err = ":ircserv 409 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " PING :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }
    
    std::string pong = "PONG " + args[1] + "\r\n";
    QueueMessage(cli, pong);
}

static std::vector<std::string> Split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void Server::Join(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 2) {
        std::string err = ":ircserv 461 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " JOIN :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }

    std::string channelList = args[1];
    std::string keyList = (args.size() > 2) ? args[2] : "";

    std::vector<std::string> channels = Split(channelList, ',');
    std::vector<std::string> keys = Split(keyList, ',');

    if (channels.size() > 15) {
        channels.resize(15);
    }

    for (size_t i = 0; i < channels.size(); ++i) {
        if (channels[i] == "0") {
            LeaveAllChannels(cli);
            continue;
        }
        std::string key = (i < keys.size()) ? keys[i] : "";
        JoinSingle(cli, channels[i], key);
    }
}

void Server::LeaveAllChannels(Client *cli) {
    for (size_t i = 0; i < channels.size(); ) {
        if (channels[i]->IsClientInChannel(cli)) {
            std::string partMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " PART " + channels[i]->GetName() + "\r\n";
            channels[i]->Broadcast(this, partMsg);
            
            channels[i]->RemoveClient(cli);
            channels[i]->RemoveInvited(cli->GetNickname());

            if (channels[i]->GetClients().empty()) {
                delete channels[i];
                channels.erase(channels.begin() + i);
                continue;
            }
        }
        i++;
    }
}

void Server::JoinSingle(Client *cli, std::string channelName, std::string key){
    if (channelName[0] != '#') {
        return;
    }

    Channel *channel = GetChannel(channelName);
    if (!channel) {
        channel = CreateChannel(channelName);
        channel->AddAdmin(cli);
    }

    if (channel->GetModeI()) {
        if (!channel->IsInvited(cli->GetNickname())) {
             std::string err = ":ircserv 473 " + cli->GetNickname() + " " + channelName + " :Cannot join channel (+i)\r\n";
             QueueMessage(cli, err);
             return;
        }
    }
    
    if (channel->GetModeL()) {
        if (channel->GetClients().size() >= channel->GetLimit()) {
            std::string err = ":ircserv 471 " + cli->GetNickname() + " " + channelName + " :Cannot join channel (+l)\r\n";
            QueueMessage(cli, err);
            return;
        }
    }
    
    if (channel->GetModeK()) {
        if (channel->GetKey() != key) {
            std::string err = ":ircserv 475 " + cli->GetNickname() + " " + channelName + " :Cannot join channel (+k)\r\n";
            QueueMessage(cli, err);
            return;
        }
    }

    if (channel->IsClientInChannel(cli))
        return;

    channel->AddClient(cli);
    
    std::string joinMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " JOIN :" + channelName + "\r\n";
    channel->Broadcast(this, joinMsg);
    
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
    QueueMessage(cli, namReply);

    std::string endNames = ":ircserv 366 " + cli->GetNickname() + " " + channelName + " :End of /NAMES list.\r\n";
    QueueMessage(cli, endNames);
    
   if (!channel->GetTopic().empty()) {
        std::string topicMsg = ":ircserv 332 " + cli->GetNickname() + " " + channelName + " :" + channel->GetTopic() + "\r\n";
        QueueMessage(cli, topicMsg);
   }
}

void Server::Privmsg(Client *cli, std::string cmd){
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token){
        args.push_back(token);
    }

    if (args.size() < 2) {
        // ERR_NEEDMOREPARAMS (461)
        std::string err = ":ircserv 461 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " PRIVMSG :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }
    
    if (args.size() < 3) {
        // ERR_NOTEXTTOSEND (412)
        std::string err = ":ircserv 412 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " :No text to send\r\n";
        QueueMessage(cli, err);
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
            std::string fullMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " PRIVMSG " + target + " :" + message + "\r\n";
            channel->Broadcast(this, fullMsg, cli->GetFd()); // Don't send back to sender
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
             std::string fullMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " PRIVMSG " + target + " :" + message + "\r\n";
             QueueMessage(dest, fullMsg);
        } else {
            // ERR_NOSUCHNICK
            std::string err = ":ircserv 401 " + cli->GetNickname() + " " + target + " :No such nick/channel\r\n";
            QueueMessage(cli, err);
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
        QueueMessage(cli, err);
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
        // ERR_NONICKNAMEGIVEN (431)
        std::string err = ":ircserv 431 * :No nickname given\r\n";
        QueueMessage(cli, err);
        return;
    }

    if (!cli->GetLoggedIn()) {
        // ERR_NOTREGISTERED (451) - must send PASS first
        std::string err = ":ircserv 451 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " NICK :You have not registered\r\n";
        QueueMessage(cli, err);
        return;
    }

    // Check collision
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->GetNickname() == args[1]) {
            // ERR_NICKNAMEINUSE
            std::string err = ":ircserv 433 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " " + args[1] + " :Nickname is already in use\r\n";
            QueueMessage(cli, err);
            return;
        }
    }

    // Set nickname
    std::string oldNick = cli->GetNickname();
    std::string newNick = args[1];
    
    if (cli->GetRegistered()) {
        // Broadcast NICK change to all channels user is in
        // :oldnick!user@host NICK :newnick
        std::string nickMsg = ":" + oldNick + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " NICK :" + newNick + "\r\n";
        
        // Find channels and broadcast
        // Note: In an optimized server we would find unique users. Here we iterate channels.
        // Also send to self
        QueueMessage(cli, nickMsg);
        
        for (size_t i = 0; i < channels.size(); i++) {
            if (channels[i]->IsClientInChannel(cli)) {
                channels[i]->Broadcast(this, nickMsg, cli->GetFd()); // Don't send relative to sender again?
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
        // ERR_NEEDMOREPARAMS (461)
        std::string err = ":ircserv 461 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " USER :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }

    if (!cli->GetLoggedIn()) {
        // ERR_NOTREGISTERED (451)
        std::string err = ":ircserv 451 " + (cli->GetNickname().empty() ? "*" : cli->GetNickname()) + " USER :You have not registered\r\n";
        QueueMessage(cli, err);
        return;
    }
    
    if (cli->GetRegistered()) {
         std::string err = ":ircserv 462 " + cli->GetNickname() + " :You may not reregister\r\n";
            QueueMessage(cli, err);
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
         QueueMessage(cli, err);
        return;
    }
    
    if (!channel->IsClientInChannel(cli)) {
        // ERR_NOTONCHANNEL
        std::string err = ":ircserv 442 " + cli->GetNickname() + " " + channelName + " :You're not on that channel\r\n";
        QueueMessage(cli, err);
        return;
    }
    
    // Broadcast PART message to channel (including user)
    // :user!user@host PART #channel
    std::string partMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " PART " + channelName + "\r\n";
    channel->Broadcast(this, partMsg);
    
    channel->RemoveClient(cli);
    channel->RemoveInvited(cli->GetNickname());
    
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
    QueueMessage(cli, welcome);

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
    QueueMessage(cli, help);
}

void Server::Kick(Client *cli, std::string cmd) {
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token) {
        args.push_back(token);
    }

    if (args.size() < 3) {
        // ERR_NEEDMOREPARAMS
        std::string err = ":ircserv 461 " + cli->GetNickname() + " KICK :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }

    std::string channelName = args[1];
    std::string targetNick = args[2];
    std::string reason = "Kicked by operator";
    
    // Check if reason is provided
    // Reconstruct reason from original cmd string to preserve spaces logic similar to privmsg
    // but here we have KICK #chan user :reason
    size_t reasonPos = cmd.find(targetNick) + targetNick.length();
    if (reasonPos < cmd.length()) {
         std::string r = cmd.substr(reasonPos);
         size_t firstChar = r.find_first_not_of(" ");
         if (firstChar != std::string::npos) {
             r = r.substr(firstChar);
             if (!r.empty() && r[0] == ':')
                r = r.substr(1);
             if (!r.empty())
                reason = r;
         }
    }

    Channel *channel = GetChannel(channelName);
    if (!channel) {
        // ERR_NOSUCHCHANNEL
        std::string err = ":ircserv 403 " + cli->GetNickname() + " " + channelName + " :No such channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    if (!channel->IsClientInChannel(cli)) {
        // ERR_NOTONCHANNEL
        std::string err = ":ircserv 442 " + cli->GetNickname() + " " + channelName + " :You're not on that channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    if (!channel->IsAdmin(cli)) {
        // ERR_CHANOPRIVSNEEDED
        std::string err = ":ircserv 482 " + cli->GetNickname() + " " + channelName + " :You're not channel operator\r\n";
        QueueMessage(cli, err);
        return;
    }

    Client *target = NULL;
    std::vector<Client*> chanClients = channel->GetClients();
    for(size_t i=0; i<chanClients.size(); i++) {
        if(chanClients[i]->GetNickname() == targetNick) {
            target = chanClients[i];
            break;
        }
    }

    if (!target) {
        // ERR_USERNOTINCHANNEL
        std::string err = ":ircserv 441 " + cli->GetNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    // Broadcast KICK
    // :admin!user@host KICK #channel target :reason
    std::string kickMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    channel->Broadcast(this, kickMsg); // Broadcast to everyone including target

    channel->RemoveClient(target);
    channel->RemoveInvited(targetNick);
    
    // Check if channel empty
    if (channel->GetClients().empty()) {
        for (size_t i = 0; i < channels.size(); i++) {
            if (channels[i] == channel) {
                channels.erase(channels.begin() + i);
                delete channel;
                break;
            }
        }
    }
}

void Server::Invite(Client *cli, std::string cmd) {
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token) {
        args.push_back(token);
    }

    if (args.size() < 3) {
        // ERR_NEEDMOREPARAMS
        std::string err = ":ircserv 461 " + cli->GetNickname() + " INVITE :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }

    std::string targetNick = args[1];
    std::string channelName = args[2];

    Channel *channel = GetChannel(channelName);
    if (!channel) {
        // ERR_NOSUCHCHANNEL
        // Wait, standard says you can invite to non-existent channel? 
        // But typically one invites to a channel they are on.
        // RFC 1459: "a user ... must be a channel operator of the channel" => channel must exist and user must be on it.
        std::string err = ":ircserv 403 " + cli->GetNickname() + " " + channelName + " :No such channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    if (!channel->IsClientInChannel(cli)) {
        // ERR_NOTONCHANNEL
         std::string err = ":ircserv 442 " + cli->GetNickname() + " " + channelName + " :You're not on that channel\r\n";
        QueueMessage(cli, err);
        return;
    }
    
    // Check operator privileges (Requirement: Ops only)
    if (!channel->IsAdmin(cli)) {
        // ERR_CHANOPRIVSNEEDED
        std::string err = ":ircserv 482 " + cli->GetNickname() + " " + channelName + " :You're not channel operator\r\n";
        QueueMessage(cli, err);
        return;
    }

    // Check if target exists
    Client *target = NULL;
    for(size_t i=0; i<clients.size(); i++) {
        if(clients[i]->GetNickname() == targetNick) {
            target = clients[i];
            break;
        }
    }
    
    if (!target) {
        // ERR_NOSUCHNICK
        std::string err = ":ircserv 401 " + cli->GetNickname() + " " + targetNick + " :No such nick/channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    if (channel->IsClientInChannel(target)) {
        // ERR_USERONCHANNEL
        std::string err = ":ircserv 443 " + cli->GetNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    // Send RPL_INVITING to issuer
    std::string inviting = ":ircserv 341 " + cli->GetNickname() + " " + targetNick + " " + channelName + "\r\n";
    QueueMessage(cli, inviting);

    // Send INVITE msg to target
    // :sender!user@host INVITE target :#channel
    std::string inviteMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    QueueMessage(target, inviteMsg);
    
    channel->AddInvited(targetNick);
}

void Server::Topic(Client *cli, std::string cmd) {
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token) {
        args.push_back(token);
    }
    
    if (args.size() < 2) {
        // ERR_NEEDMOREPARAMS
        std::string err = ":ircserv 461 " + cli->GetNickname() + " TOPIC :Not enough parameters\r\n";
        QueueMessage(cli, err);
        return;
    }
    
    std::string channelName = args[1];
    Channel *channel = GetChannel(channelName);
    if (!channel) {
         std::string err = ":ircserv 403 " + cli->GetNickname() + " " + channelName + " :No such channel\r\n";
            QueueMessage(cli, err);
         return;
    }
    
    if (!channel->IsClientInChannel(cli)) {
        std::string err = ":ircserv 442 " + cli->GetNickname() + " " + channelName + " :You're not on that channel\r\n";
        QueueMessage(cli, err);
        return;
    }

    // Check if viewing or setting
    if (args.size() == 2) {
        // View Topic
        if (channel->GetTopic().empty()) {
            std::string msg = ":ircserv 331 " + cli->GetNickname() + " " + channelName + " :No topic is set\r\n";
            QueueMessage(cli, msg);
        } else {
            std::string msg = ":ircserv 332 " + cli->GetNickname() + " " + channelName + " :" + channel->GetTopic() + "\r\n";
            QueueMessage(cli, msg);
        }
    } else {
        // Set Topic
        // Check +t mode
        if (channel->GetModeT() && !channel->IsAdmin(cli)) {
             std::string err = ":ircserv 482 " + cli->GetNickname() + " " + channelName + " :You're not channel operator\r\n";
             QueueMessage(cli, err);
             return;
        }
        
        std::string newTopic = "";
        // Extract topic (handle spaces)
        size_t topicPos = cmd.find(channelName) + channelName.length();
        if(topicPos != std::string::npos) {
             std::string t = cmd.substr(topicPos);
             size_t first = t.find_first_not_of(" ");
             if(first != std::string::npos) {
                t = t.substr(first);
                if(!t.empty() && t[0] == ':')
                   t = t.substr(1);
                newTopic = t;
             }
        }
        
        channel->SetTopic(newTopic);
        // Broadcast TOPIC change
        std::string topicMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
        channel->Broadcast(this, topicMsg);
    }
}

void Server::Mode(Client *cli, std::string cmd) {
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while(iss >> token) {
        args.push_back(token);
    }

    if (args.size() < 2) {
         std::string err = ":ircserv 461 " + cli->GetNickname() + " MODE :Not enough parameters\r\n";
            QueueMessage(cli, err);
        return;
    }
    
    std::string target = args[1];
    
    if (target[0] != '#') {
        // User mode - ignore for 42 subject mostly, or return error
         // std::cout << "User mode not implemented" << std::endl;
         return;
    }
    
    Channel *channel = GetChannel(target);
    if (!channel) {
         std::string err = ":ircserv 403 " + cli->GetNickname() + " " + target + " :No such channel\r\n";
            QueueMessage(cli, err);
         return;
    }
    
    if (args.size() == 2) {
        // RPL_CHANNELMODEIS
        std::string modes = channel->GetModeString();
        // Append arguments like key and limit if set (optional but good)
        std::string msg = ":ircserv 324 " + cli->GetNickname() + " " + target + " " + modes + "\r\n";
        QueueMessage(cli, msg);
        return;
    }
    
    // Set/Unset Mode
    if (!channel->IsClientInChannel(cli)) {
        // ERR_NOTONCHANNEL? OR just check operator?
        // Usually need to be on channel to set mode?
        // Let's assume yes.
    }
    
    if (!channel->IsAdmin(cli)) {
         std::string err = ":ircserv 482 " + cli->GetNickname() + " " + target + " :You're not channel operator\r\n";
            QueueMessage(cli, err);
         return;
    }
    
    std::string modeString = args[2];
    bool adding = true;
    size_t argIdx = 3; // Index for mode arguments
    std::string changes = ""; // To reconstruct what actually changed for broadcast
    
    for(size_t i=0; i<modeString.length(); i++) {
        char mode = modeString[i];
        
        if (mode == '+') {
            adding = true;
            changes += "+";
        } else if (mode == '-') {
            adding = false;
            changes += "-";
        } else if (mode == 'i') {
            channel->SetModeI(adding);
            changes += "i";
        } else if (mode == 't') {
            channel->SetModeT(adding);
            changes += "t";
        } else if (mode == 'k') {
            if (adding) {
                if (argIdx < args.size()) {
                    channel->SetKey(args[argIdx++]);
                    changes += "k";
                }
            } else {
                channel->RemoveKey();
                changes += "k";
            }
        } else if (mode == 'l') {
            if (adding) {
                if (argIdx < args.size()) {
                    channel->SetLimit(atoi(args[argIdx++].c_str()));
                    changes += "l";
                }
            } else {
                channel->RemoveLimit();
                changes += "l";
            }
        } else if (mode == 'o') {
            if (argIdx < args.size()) {
                std::string targetNick = args[argIdx++];
                // Find client in channel
                std::vector<Client*> chClients = channel->GetClients();
                Client *targetCli = NULL;
                for(size_t j=0; j<chClients.size(); j++) {
                    if(chClients[j]->GetNickname() == targetNick) {
                        targetCli = chClients[j];
                        break;
                    }
                }
                if (targetCli) {
                    if (adding) channel->AddAdmin(targetCli);
                    else channel->RemoveAdmin(targetCli);
                    changes += "o " + targetNick; // Add argument to change check
                    // Wait, simplistic reconstruction:
                    // MODE #chan +o user
                }
            }
        }
    }
    
    // Broadcast changes
    // Simplified broadcast string construction. Real IRC servers are more precise.
    // :nick!user@host MODE #channel +i
    if (!changes.empty()) {
        std::string modeMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " MODE " + target + " " + modeString;
        // Append args used?
         // For now just sending what user sent, assuming typical usage one by one or simple combo
         // For precise 'changes' string building, it's more complex.
         // Let's just echo back the successful command essentially.
         // But we should only echo if valid.
        
         // Better implementation:
        std::string finalMsg = ":" + cli->GetNickname() + "!" + cli->GetUsername() + "@" + cli->getIpAdd() + " MODE " + target + " " + modeString;
        for (size_t i = 3; i < argIdx; ++i) { // Append consumed args
             if (i < args.size())
                finalMsg += " " + args[i];
        }
        finalMsg += "\r\n";
        channel->Broadcast(this, finalMsg);
    }
}
