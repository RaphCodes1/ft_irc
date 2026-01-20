#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "ircserv.hpp"
#include "Client.hpp"
#include <cstdlib>

class Client;
class Server;

class Channel
{
    private:
        std::string Name;
        std::vector<Client*> Clients;
        std::vector<Client*> Admins;
        std::string Topic;
        std::string Key;
        size_t Limit; // 0 = unlimited
        
        bool ModeI; // Invite only
        bool ModeT; // Topic restricted
        bool ModeK; // Key needed
        bool ModeL; // User limit
        
        std::vector<std::string> InvitedNicks;

    public:
        Channel(std::string name);
        ~Channel();

        std::string GetName() const { return Name; }
        
        // Topic
        void SetTopic(std::string topic) { Topic = topic; }
        std::string GetTopic() const { return Topic; }

        // Key
        void SetKey(std::string key) { Key = key; ModeK = true; }
        std::string GetKey() const { return Key; }
        void RemoveKey() { Key = ""; ModeK = false; }

        // Limit
        void SetLimit(size_t limit) { Limit = limit; ModeL = true; }
        size_t GetLimit() const { return Limit; }
        void RemoveLimit() { Limit = 0; ModeL = false; }

        // Modes
        void SetModeI(bool val) { ModeI = val; }
        bool GetModeI() const { return ModeI; }
        void SetModeT(bool val) { ModeT = val; }
        bool GetModeT() const { return ModeT; }
        bool GetModeK() const { return ModeK; }
        bool GetModeL() const { return ModeL; }
        
        std::string GetModeString();
        
        void AddClient(Client *cli);
        void RemoveClient(Client *cli);
        bool IsClientInChannel(Client *cli);
        
        void AddAdmin(Client *cli);
        void RemoveAdmin(Client *cli);
        bool IsAdmin(Client *cli);

        void AddInvited(std::string nick);
        bool IsInvited(std::string nick);

        void Broadcast(Server *server, std::string msg, int excludeFd = -1);
        std::vector<Client*> GetClients() const { return Clients; }
};

#endif
