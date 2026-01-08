Core Functionality
Multi-client support with non-blocking I/O using poll()
Password authentication for server access
User registration (NICK/USER commands)
Channel operations (JOIN/PART/KICK/INVITE)
Message handling (PRIVMSG to users and channels)
Channel management (TOPIC/MODE commands)
Graceful shutdown with SIGINT handling
IRC Commands Implemented
PASS - Server password authentication
NICK - Set or change nickname
USER - User registration
JOIN - Join channels
PART - Leave channels
PRIVMSG - Send messages to users/channels
KICK - Remove users from channels (operator only)
INVITE - Invite users to channels (operator only)
TOPIC - View/set channel topic
MODE - Set channel modes (i/t/k/o/l)
QUIT - Disconnect from server
Channel Features
Channel operators with special privileges
Channel modes: invite-only (i), topic restriction (t), key protection (k), user limit (l)
User limit enforcement
Invite-only channels
Key-protected channels