#include "ircserv.hpp"

Client::Client() : Fd(-1), Registered(false), LoggedIn(false) {}

Client::Client(int fd, std::string ip) : Fd(fd), IPadd(ip), Registered(false), LoggedIn(false) {}

Client::~Client() {}

int Client::GetFd() const { return Fd; }
std::string Client::GetIp() const { return IPadd; }
std::string Client::GetNick() const { return Nickname; }
std::string Client::GetUser() const { return Username; }
std::string Client::GetReal() const { return Realname; }
std::string Client::GetBuffer() const { return buffer; }
bool Client::GetRegistered() const { return Registered; }
bool Client::GetLoggedIn() const { return LoggedIn; }

void Client::setFd(int fd) { Fd = fd; }
void Client::setIpAdd(std::string ipadd) { IPadd = ipadd; }
void Client::setNick(std::string nick) { Nickname = nick; }
void Client::setUser(std::string user) { Username = user; }
void Client::setReal(std::string real) { Realname = real; }
void Client::setRegistered(bool val) { Registered = val; }
void Client::setLoggedIn(bool val) { LoggedIn = val; }

void Client::AddBuffer(std::string data) { buffer += data; }
void Client::ClearBuffer() { buffer.clear(); }
