#include <netdb.h>
#include <iostream>
#include <string>
#include <string.h>
#include <arpa/inet.h>

int main(){

    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if(listening == -1){
        std::cerr << "cant create a socket" << std::endl;
        return -1;
    }


    






}