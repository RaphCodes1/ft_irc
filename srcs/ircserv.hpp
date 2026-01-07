#ifndef IRCSERV_HPP
#define IRCSERV_HPP

#include <iostream>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <csignal>
#include <cstring>
#include <sstream>
#include <cctype>

#include "Server.hpp"
#include "Client.hpp"
#include "utils.hpp"

class Server;
class Client;

#endif
