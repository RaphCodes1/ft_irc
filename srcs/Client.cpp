#include "ircserv.hpp"

Client::Client(int fd, const std::string &hostname):Fd(fd), IPadd(hostname), Nickname(""), Username(""), Realname(""), Registered(false), LoggedIn(false){};

Client::~Client(){}
