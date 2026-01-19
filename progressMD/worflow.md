alright lets decypher this thing....


int main 
takes in argument in ac and av

make the Server class with validated port and pass

then we go to serverInit() method from Server

-------SerSocket()--------

first function in serverInit()

this function sets up IRC server listening socket and registers it with poll

"POLL" is a system call that lets you check multiple file descriptors(sockets, stdin, etc.)

**ADDRESS SETUP**

struct sockaddr_in addr;
addr.sin_family = AF_INET;          // IPv4
addr.sin_addr.s_addr = INADDR_ANY;  // any address
addr.sin_port = htons(this->Port);  // host -> network byte order

sockaddr_in holds the IP/port the server will bind to
AF_INET means IPv4, INADDR_ANY means “accept connections on all local interfaces”, and htons converts your Port from host endianness to network (big endian) as required by sockets.

big endian is storing multi-byte numbers in memory where the most significant byte comes first (at the lowest address)

32‑bit value "0x12 34 56 78", 
big endian "12 34 56 78" 
converts in that order.

**SOCKET CREATION**

SerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
if (SerSocketFd < 0)
    throw(std::runtime_error("failed to create socket"));

creates a TCP socket (stream) for IPv4

TCP (Transmission Control Protocol) is a connection‑oriented protocol:
the client and server perform a handshake to establish a connection, then TCP ensures packets are retransmitted if lost and delivered in order.

A stream socket (type SOCK_STREAM) is the API abstraction that uses TCP; you read and write it like a continuous byte stream, without fixed message boundaries, and TCP guarantees reliability and in‑order delivery.

**REUSE ADDRESS OPTION**

int opt = 1;
if (setsockopt(SerSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    throw(std::runtime_error("setsockopt failed"));

SO_REUSEADDR allows rebinding the same port quickly after a restart, instead of waiting for TIME_WAIT.

**BIND AND LISTEN**

if (bind(SerSocketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    throw(std::runtime_error("bind failed"));

if (listen(SerSocketFd, SOMAXCONN) < 0)
    throw(std::runtime_error("listen failed"));

bind attaches the socket to the IP/port you set in addr, making the OS route incoming packets for that port to this socket.

listen turns it into a passive listening socket; SOMAXCONN lets the OS choose the maximum backlog of pending connections.

**REGISTER IN THE POLL SET**

struct pollfd NewPollFd;
NewPollFd.fd = SerSocketFd;   // server socket
NewPollFd.events = POLLIN;    // watch for readable events (new connections)
NewPollFd.revents = 0;
fds.push_back(NewPollFd);

You wrap the server socket in a pollfd so poll() can monitor it.

POLLIN means you care when it becomes readable; for a listening socket that indicates there is at least one pending connection and you should call accept()

------------------------------------------------

-----------------ServerInit()------------------

after creating and binding the server socket, we start the main loop for the Server now

**CHECK SIGNAL OR TIMEOUT**
if((poll(&fds[0],fds.size(), -1) == -1) || (Server::Signal == true))
        throw(std::runtime_error("shutting down..."));

if poll returns -1 (timeout) it throws a runtime_error shutdown.


**FOR LOOP**
for(size_t i = 0; i < fds.size(); i++){
    if(fds[i].revents & POLLIN)
    {

Iterates over every pollfd in fds.

means “run this block only if poll says this descriptor is ready to read.”

each pollfd has events and revents
events = what you ask to watch (e.g. POLLIN for readable).

revents = what actually happened on that fd after poll returns; it is a bitmask of flags like POLLIN, POLLERR, etc.

POLLIN is a flag meaning “this fd is readable now”.

For:

A listening socket: POLLIN means there is at least one pending connection → you should call accept.

A client socket: POLLIN means there is data available to read, or read/recv will return 0 if the peer closed.

& here is bitwise AND.

Expression: fds[i].revents & POLLIN

revents is a bitmask; POLLIN is a single bit.

If the POLLIN bit is set in revents, the result is non‑zero → condition is true.

If that bit is not set, the result is 0 → condition is false.

**RUN ACCEPT NEW CLIENT**

if(fds[i].fd == SerSocketFd)
    AcceptNewClient();

If the ready fd is the server socket (SerSocketFd), it calls AcceptNewClient().

**RECIEVE NEW DATA**

else
    {
        if (ReceiveNewData(fds[i].fd)) {
            // Client disconnected and removed from fds
            // Current 'i' now points to the next element (or invalid)
            // Decrement i so loop increment moves to correct next element
            i--;
        }
    }

ReceiveNewData(fds[i].fd) likely:

Calls recv/read on that client.

Parses and handles IRC commands.

If the client disconnected or must be kicked, it removes that client’s pollfd entry from fds.


**EXCEPTION**

throws runtime error when an error occurs in the loop

closes the SerSocketFd and the fds of the clients.

------------AcceptNewClient()-------------

this function is called when the poll descriptor is ready to read

**ACCEPTING THE CONNECTION**

struct sockaddr_in addr;
struct pollfd NewPoll;
socklen_t len = sizeof(addr);

int incofd = accept(SerSocketFd, (struct sockaddr *)&addr, &len);
if(incofd < 0) {
    std::cerr << "Accept failed" << std::endl;
    return;
}

accept takes the listening socket SerSocketFd and returns a new socket descriptor incofd that represents one client connection.

addr and len are filled with the client’s address (IP and port); on error, accept returns < 0, so the function logs and returns.

why is (struct sockaddr *) cast being used

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

It expects a pointer to a generic struct sockaddr, because it must also support other address families (IPv6, Unix domain, etc.).

sockaddr_in is layout‑compatible with sockaddr at the beginning, so you can safely treat the address as a struct sockaddr * when calling accept.

**MAKING THE CLIENT SOCKET NON-BLOCKING**

if(fcntl(incofd, F_SETFL, O_NONBLOCK) < 0)
{
    std::cerr << "fcntl() failed" << std::endl;
    return;
}

fcntl with F_SETFL and O_NONBLOCK marks this client socket as non‑blocking, meaning recv/send will return immediately instead of hanging the whole server if there is no data yet.


