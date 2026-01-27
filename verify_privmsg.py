import socket
import time
import sys

def verify_privmsg_error():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 6667))
    
    # Register
    sock.send(b"PASS pass\r\n")
    sock.send(b"NICK tester\r\n")
    sock.send(b"USER tester 0 * :Tester\r\n")
    
    time.sleep(1) # Wait for welcome
    
    # Send PRIVMSG to non-existent user
    sock.send(b"PRIVMSG nonexistent :hello\r\n")
    
    # Read response loop
    sock.setblocking(0)
    total_response = ""
    start_time = time.time()
    while time.time() - start_time < 2:
        try:
            chunk = sock.recv(4096).decode('utf-8')
            if chunk:
                total_response += chunk
                print(f"Received chunk: {chunk}")
        except:
            time.sleep(0.1)
            
    response = total_response
    
    if "401" in response and "nonexistent" in response:
        print("SUCCESS: Received ERR_NOSUCHNICK (401)")
    else:
        print("FAILURE: Did not receive expected error")
        sys.exit(1)

    sock.close()

if __name__ == "__main__":
    verify_privmsg_error()
