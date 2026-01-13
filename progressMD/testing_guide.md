# FT_IRC Testing Guide

This guide details how to start the server, connect using different clients, and verify all implemented functionalities.

## 1. Building and Starting
First, verify the project builds cleanly.
```bash
make re
```

Start the server specifying a listening port and a password.
```bash
./ircserv 6667 password
```

---

## 2. Connecting to Server

### Option A: Using `netcat` (Raw Protocol)
Useful for testing exact protocol responses and error codes.

1.  **Connect**:
    ```bash
    nc localhost 6667
    ```
2.  **Authenticate & Register** (Type these lines):
    ```text
    PASS password
    NICK tester
    USER tester 0 * :Real Name
    ```
    *Expected output*: `001 Welcome to the ft_irc Network...`

### Option B: Using `irssi` (IRC Client)
Standard client for usability testing.

1.  **Start Irssi**:
    ```bash
    irssi
    ```
2.  **Connect**:
    ```text
    /connect localhost 6667 password
    ```
    *Note: If you have a default nick, it will be used. You can change it with `/nick newname`.*

---

## 3. Feature Test Cases

### A. Core Communication
| Feature | Client Command | Netcat Command | Expected Result |
| :--- | :--- | :--- | :--- |
| **Ping/Pong** | Automatic | `PING :test` | Server replies `PONG :test`. |
| **Private Msg** | `/msg other Hello` | `PRIVMSG other :Hello` | Target receives message. |
| **Quit** | `/quit` | `QUIT :Bye` | Client disconnects, server cleans up. |

### B. Channel Operations
*Pre-requisite: Client A (Op) and Client B connected.*

#### 1. Join & Part
*   **Command**: `/join #chan` | `JOIN #chan`
*   **Test**:
    1.  A joins `#test` (becomes Operator `@`).
    2.  B joins `#test`.
    3.  A sees B join.
    4.  B leaves: `/part #test` | `PART #test`.

#### 2. Topic
*   **Command**: `/topic #chan :Content` | `TOPIC #chan :Content`
*   **Test**:
    1.  A sets topic: `/topic #test :Welcome to 42`.
    2.  B sees topic change notification.
    3.  B checks topic: `/topic #test`.

#### 3. Kick (Op Only)
*   **Command**: `/kick #chan user` | `KICK #chan user :Reason`
*   **Test**:
    1.  B joins `#test`.
    2.  A kicks B: `/kick #test B`.
    3.  B is removed from channel.
    4.  B tries to kick A -> Fails (Permissions).

#### 4. Invite (Op Only)
*   **Command**: `/invite user #chan` | `INVITE user #chan`
*   **Test**:
    1.  B is NOT in `#test`.
    2.  A invites B: `/invite B #test`.
    3.  B receives invitation.
    4.  B joins `#test` (bypasses +i if set).

### C. Channel Modes (Advanced)
*Use `/mode #chan +<flag> <args>`*

#### 1. Invite Only (+i)
*   **Test**:
    1.  A sets mode: `/mode #test +i`.
    2.  B tries `/join #test` -> **Fails** (Cannot join channel (+i)).
    3.  A invites B: `/invite B #test`.
    4.  B tries `/join #test` -> **Success**.

#### 2. Topic Lock (+t)
*   **Test**:
    1.  A sets mode: `/mode #test +t`.
    2.  B (non-op) tries to change topic: `/topic #test :Hacked` -> **Fails** (Not channel operator).
    3.  A (op) changes topic -> **Success**.

#### 3. Key/Password (+k)
*   **Test**:
    1.  A sets key: `/mode #test +k secret`.
    2.  B tries `/join #test` -> **Fails** (Bad channel key).
    3.  B tries `/join #test wrong` -> **Fails**.
    4.  B tries `/join #test secret` -> **Success**.

#### 4. User Limit (+l)
*   **Test**:
    1.  A sets limit: `/mode #test +l 1`.
    2.  B tries `/join #test` -> **Fails** (Channel full).
    3.  A removes limit: `/mode #test -l`.
    4.  B tries `/join #test` -> **Success**.

#### 5. Operator Status (+o)
*   **Test**:
    1.  A gives Op to B: `/mode #test +o B`.
    2.  B can now Kick/Mode/Topic.
    3.  A takes Op from B: `/mode #test -o B`.
