#include "device.hpp"

 int Device::setSocket(void){
        if ((this->sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket create failed");
        return -1;
    }
    if (setsockopt(this->sock_fd, SOL_SOCKET, SO_REUSEADDR, &(this->opt), sizeof((this->opt)))) {
        perror("setsockopt");
        return -1;
    }
    this->addr.sin_family = AF_INET;
    this->addr.sin_addr.s_addr = INADDR_ANY;
    this->addr.sin_port = htons(8080);

    // // Forcefully attaching socket to the port 8080
    if (bind(this->sock_fd, (struct sockaddr *)&(this->addr), sizeof(this->addr))<0) {
        perror("bind failed");
        return -1;
    }

    if (listen(this->sock_fd, MAX_REQUEST_NUMBER) < 0) {
        perror("listen");
        return -1;
    }
}