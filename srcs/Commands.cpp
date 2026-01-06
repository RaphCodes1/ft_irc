#include "ircserv.hpp"
#include <sstream>

void Server::Parse(Client *cli, std::string cmd)

{
    std::stringstream ss(cmd);
    std::string command;
    ss >> command;
    
    if (command.empty())
        return;

    if (command == "CAP")
    {
        // Irssi sends CAP LS 302, we can ignore or reply
        // For basic irc, ignoring is often fine, or reply "CAP * LS :"
    }
    else if (command == "PASS")
    {
        std::string pass;
        ss >> pass;
        // Verify password
        if (pass.empty())
        {
            // ERR_NEEDMOREPARAMS
            return;
        }
        if (pass != Password)
        {
            // ERR_PASSWDMISMATCH
            std::cout << "Wrong Password" << std::endl;
            return;
        }
        cli->setLoggedIn(true);
        std::cout << "Client " << cli->GetFd() << " Password Correct" << std::endl;
    }
    else if (command == "NICK")
    {
        if (!cli->GetLoggedIn())
        {
            // ERR_NOTREGISTERED (implied check pass logic needed if pass mandatory)
             // But actually, we need to check if Pass was sent IF password is set.
             // My logic sets LoggedIn = true only on correct PASS.
             std::cout << "Client " << cli->GetFd() << " Not Logged In" << std::endl;
             // Send error
             return;
        }
        
        std::string nick;
        ss >> nick;
        if (nick.empty())
            return;
        
        cli->setNick(nick);
        // Collision check needed? Yes.
        
        // If USER also set, finish registration
        if (!cli->GetUser().empty() && !cli->GetNick().empty() && !cli->GetRegistered())
        {
            cli->setRegistered(true);
            std::cout << "Client " << cli->GetFd() << " Registered!" << std::endl;
            // Send RPL_WELCOME
             std::string welcome = ":localhost 001 " + cli->GetNick() + " :Welcome to the ft_irc Network, " + cli->GetNick() + "\r\n";
             send(cli->GetFd(), welcome.c_str(), welcome.length(), 0);
        }
    }
    else if (command == "USER")
    {
        if (!cli->GetLoggedIn())
        {
             std::cout << "Client " << cli->GetFd() << " Not Logged In" << std::endl;
             return;
        }
        std::string user, mode, unused, realname;
        ss >> user >> mode >> unused;
        // The rest is realname (starts with :)
        std::getline(ss, realname);
        if (!realname.empty() && realname[0] == ' ') realname.erase(0, 1);
        if (!realname.empty() && realname[0] == ':') realname.erase(0, 1);

        cli->setUser(user);
        cli->setReal(realname);

        if (!cli->GetUser().empty() && !cli->GetNick().empty() && !cli->GetRegistered())
        {
            cli->setRegistered(true);
            std::cout << "Client " << cli->GetFd() << " Registered!" << std::endl;
            // Send RPL_WELCOME
            std::string welcome = ":localhost 001 " + cli->GetNick() + " :Welcome to the ft_irc Network, " + cli->GetNick() + "\r\n";
            send(cli->GetFd(), welcome.c_str(), welcome.length(), 0);
        }
    }
    else if (command == "JOIN")
    {
        if (!cli->GetRegistered())
            return; // Send ERR_NOTREGISTERED
            
        std::string chanName;
        ss >> chanName;
        
        if (chanName.empty())
            return; // ERR_NEEDMOREPARAMS
            
        // Check if channel exists, if not create
        int chanIdx = -1;
        for(size_t i = 0; i < channels.size(); i++)
        {
            if(channels[i].GetName() == chanName)
            {
                chanIdx = i;
                break;
            }
        }
        
        if (chanIdx == -1)
        {
            Channel newChan(chanName);
            newChan.AddClient(*cli);
            channels.push_back(newChan);
            chanIdx = channels.size() - 1;
        }
        else
        {
            // Check if already in channel?
             channels[chanIdx].AddClient(*cli);
        }
        
        // Broadcast JOIN to all in channel
        std::string joinMsg = ":" + cli->GetNick() + "!" + cli->GetUser() + "@localhost JOIN " + chanName + "\r\n";
        std::vector<Client> &chClients = channels[chanIdx].GetClients();
        for(size_t i = 0; i < chClients.size(); i++)
        {
            send(chClients[i].GetFd(), joinMsg.c_str(), joinMsg.length(), 0);
        }
        
        // Topic (332) - omit for now if empty
        // Names (353, 366)
        std::string namesList = ":localhost 353 " + cli->GetNick() + " = " + chanName + " :";
        for(size_t i = 0; i < chClients.size(); i++)
        {
            namesList += chClients[i].GetNick() + " ";
        }
        namesList += "\r\n";
        send(cli->GetFd(), namesList.c_str(), namesList.length(), 0);
        
        std::string endNames = ":localhost 366 " + cli->GetNick() + " " + chanName + " :End of /NAMES list.\r\n";
        send(cli->GetFd(), endNames.c_str(), endNames.length(), 0);

        std::cout << "Client " << cli->GetFd() << " Joined " << chanName << std::endl;
    }
    else if (command == "PRIVMSG")
    {
        if (!cli->GetRegistered())
            return;

        std::string target, message;
        ss >> target;
        
        // Message is rest of line
        std::getline(ss, message);
        if (!message.empty() && message[0] == ' ') message.erase(0, 1);
        if (!message.empty() && message[0] == ':') message.erase(0, 1);

        if (target.empty()) return; // ERR_NORECIPIENT
        if (message.empty()) return; // ERR_NOTEXTTOSEND
        
        if (target[0] == '#') // Channel
        {
            // Find channel
            int chanIdx = -1;
            for(size_t i = 0; i < channels.size(); i++)
            {
                if(channels[i].GetName() == target)
                {
                    chanIdx = i;
                    break;
                }
            }
            if (chanIdx != -1)
            {
                std::string fullMsg = ":" + cli->GetNick() + "!" + cli->GetUser() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
                 std::vector<Client> &chClients = channels[chanIdx].GetClients();
                for(size_t i = 0; i < chClients.size(); i++)
                {
                    if (chClients[i].GetFd() != cli->GetFd())
                        send(chClients[i].GetFd(), fullMsg.c_str(), fullMsg.length(), 0);
                }
            }
            else
            {
                // ERR_NOSUCHCHANNEL
                std::string err = ":localhost 403 " + cli->GetNick() + " " + target + " :No such channel\r\n";
                send(cli->GetFd(), err.c_str(), err.length(), 0);
            }
        }
        else // User (Direct Message)
        {
             // Find user
             bool found = false;
             for(size_t i = 0; i < clients.size(); i++)
             {
                 if (clients[i].GetNick() == target)
                 {
                      std::string fullMsg = ":" + cli->GetNick() + "!" + cli->GetUser() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
                      send(clients[i].GetFd(), fullMsg.c_str(), fullMsg.length(), 0);
                      found = true;
                      break;
                 }
             }
             if (!found)
             {
                 // ERR_NOSUCHNICK
                  std::string err = ":localhost 401 " + cli->GetNick() + " " + target + " :No such nick/channel\r\n";
                  send(cli->GetFd(), err.c_str(), err.length(), 0);
             }
        }
    }
    else if (command == "PING")
    {
        std::string token;
        ss >> token;
        if (token.empty())
            return;
        std::string pong = ":localhost PONG localhost :" + token + "\r\n";
        send(cli->GetFd(), pong.c_str(), pong.length(), 0);
    }
    else if (command == "KICK")
    {
        std::string chanName, targetNick, reason;
        ss >> chanName >> targetNick;
        std::getline(ss, reason);
        if (!reason.empty() && reason[0] == ' ') reason.erase(0, 1);
        if (!reason.empty() && reason[0] == ':') reason.erase(0, 1);
        if (reason.empty()) reason = "Kicked by operator";

        if (chanName.empty() || targetNick.empty()) return; // ERR_NEEDMOREPARAMS

        int chanIdx = -1;
        for(size_t i = 0; i < channels.size(); i++)
        {
            if(channels[i].GetName() == chanName)
            {
                chanIdx = i;
                break;
            }
        }
        if (chanIdx == -1) return; // ERR_NOSUCHCHANNEL

        // Check if executor is operator (omitted for basic flow, but needed for 42)
        // Check if target is in channel
        // Perform Kick
        
        std::string kickMsg = ":" + cli->GetNick() + "!" + cli->GetUser() + "@localhost KICK " + chanName + " " + targetNick + " :" + reason + "\r\n";
        
        std::vector<Client> &chClients = channels[chanIdx].GetClients();

        for(size_t i = 0; i < chClients.size(); i++)
        {
             send(chClients[i].GetFd(), kickMsg.c_str(), kickMsg.length(), 0);
        }

        // Actually, if we remove inside loop (via RemoveClient which acts on vector), the iterator `i` might be invalid or skip.
        // Better way: Find target fd first.
        int targetFd = -1;
        for(size_t i = 0; i < chClients.size(); i++)
        {
            if (chClients[i].GetNick() == targetNick)
            {
                targetFd = chClients[i].GetFd();
                break;
            }
        }
        
        if (targetFd != -1)
        {
             // Send to all
             for(size_t i = 0; i < chClients.size(); i++)
                send(chClients[i].GetFd(), kickMsg.c_str(), kickMsg.length(), 0);
             
             channels[chanIdx].RemoveClient(targetFd);
        }
        else
        {
            // ERR_USERNOTINCHANNEL
        }
    }
    else if (command == "INVITE")
    {
        std::string targetNick, chanName;
        ss >> targetNick >> chanName;
        
        if (targetNick.empty() || chanName.empty()) return; // ERR_NEEDMOREPARAMS
        
        // Find target user
        int targetFd = -1;
         for(size_t i = 0; i < clients.size(); i++)
         {
             if (clients[i].GetNick() == targetNick)
             {
                 targetFd = clients[i].GetFd();
                 break;
             }
         }
         
         if (targetFd == -1) return; // ERR_NOSUCHNICK
         
         // Send INVITE msg to target
         std::string invMsg = ":" + cli->GetNick() + "!" + cli->GetUser() + "@localhost INVITE " + targetNick + " " + chanName + "\r\n";
         send(targetFd, invMsg.c_str(), invMsg.length(), 0);
         
         // Helper: RPL_INVITING to sender?
    }
    else if (command == "TOPIC")
    {
        std::string chanName, topic;
        ss >> chanName;
        std::getline(ss, topic);
        if (!topic.empty() && topic[0] == ' ') topic.erase(0, 1);
        
        if (chanName.empty()) return;
        
        int chanIdx = -1;
        for(size_t i = 0; i < channels.size(); i++)
        {
            if(channels[i].GetName() == chanName)
            {
                chanIdx = i;
                break;
            }
        }
        if (chanIdx == -1) return; // ERR_NOSUCHCHANNEL

        if (topic.empty())
        {
            // View Topic
            // RPL_TOPIC or RPL_NOTOPIC
             std::string topicMsg = ":localhost 332 " + cli->GetNick() + " " + chanName + " :Current Topic\r\n";
             send(cli->GetFd(), topicMsg.c_str(), topicMsg.length(), 0);
        }
        else
        {
            if (topic[0] == ':') topic.erase(0, 1);
            // Set Topic
            // channels[chanIdx].SetTopic(topic);
            std::string topicChange = ":" + cli->GetNick() + "!" + cli->GetUser() + "@localhost TOPIC " + chanName + " :" + topic + "\r\n";
            std::vector<Client> &chClients = channels[chanIdx].GetClients();
            for(size_t i = 0; i < chClients.size(); i++)
                send(chClients[i].GetFd(), topicChange.c_str(), topicChange.length(), 0);
        }
    }
    else if (command == "MODE")

    {
         std::string target, mode;
         ss >> target >> mode;
         if (!target.empty() && !mode.empty())
         {
             // For now just acknowledge or ignore
         }
    }
    else if (command == "QUIT")
    {
        std::cout << "Client " << cli->GetFd() << " Quit" << std::endl;
        ClearClients(cli->GetFd());
        close(cli->GetFd());
    }
    else
    {
        std::cout << "Unknown command: " << command << std::endl;
    }

}

