#include "ircserv.hpp"

Client::Client(int fd, const std::string &hostname):Fd(fd), IPadd(hostname){};

Client::~Client(){}
