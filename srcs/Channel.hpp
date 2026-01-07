#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "ircserv.hpp"
#include "Client.hpp"

class Client;

class Channel
{
    private:
        std::string Name;
        std::vector<Client*> Clients;
        // std::string Topic; // For later

    public:
        Channel(std::string name);
        ~Channel();

        std::string GetName() const { return Name; }
        
        void AddClient(Client *cli);
        void RemoveClient(Client *cli);
        bool IsClientInChannel(Client *cli); // Helper
        
        void Broadcast(std::string msg, int excludeFd = -1);
        std::vector<Client*> GetClients() const { return Clients; }
};

#endif
