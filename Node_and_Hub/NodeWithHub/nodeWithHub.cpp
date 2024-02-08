#include "./nodeWithHub.hpp"

NodeWithHub::NodeWithHub(const std::string& name, const Department& dep, const std::string& myIp){
    this->nodeInfo.userNmae = name;
    this->nodeInfo.department = dep;
    // this->nodeInfo.currentPath = std::filesystem::current_path();
    this->myIp = myIp;
}

 int NodeWithHub::setSocketForLocal(void){
    if ((this->local_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket create failed");
        return -1;
    }
    if (setsockopt(this->sock_fd, SOL_SOCKET, SO_REUSEADDR, &(this->opt), sizeof((this->opt)))) {
        perror("setsockopt");
        return -1;
    }
    this->local_addr.sin_family = AF_INET;
    this->local_addr.sin_addr.s_addr = INADDR_ANY;
    this->local_addr.sin_port = htons(8080);

    // // Forcefully attaching socket to the port 8080
    if (bind(this->local_socket, (struct sockaddr *)&(this->local_addr), sizeof(this->local_addr))<0) {
        perror("bind failed");
        return -1;
    }

    if (listen(this->local_socket, MAX_REQUEST_NUMBER) < 0) {
        perror("listen");
        return -1;
    }
}

int NodeWithHub::setSocketForNode(void){
    if ((this->node_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket create failed");
        return -1;
    }
    if (setsockopt(this->node_socket, SOL_SOCKET, SO_REUSEADDR, &(this->opt), sizeof((this->opt)))) {
        perror("setsockopt");
        return -1;
    }
    this->local_addr.sin_family = AF_INET;
    this->local_addr.sin_addr.s_addr = INADDR_ANY;
    this->local_addr.sin_port = htons(8080);

    // // Forcefully attaching socket to the port 8080
    if (bind(this->node_socket, (struct sockaddr *)&(this->node_addr), sizeof(this->node_addr))<0) {
        perror("bind failed");
        return -1;
    }

    if (listen(this->node_socket, MAX_REQUEST_NUMBER) < 0) {
        perror("listen");
        return -1;
    }
}

int NodeWithHub::run(void){
    // if (this->receiveIpFromHub() == -1){
    //     std::cerr << "Receiving Ip from hub is failed" << std::endl;
    //     return -1;
    // }
    if (this->broadcastIpToNode() == -1){
        std::cerr << "Broadcasting is failed" << std::endl;
        return -1;
    }
    if (this->setSocketForNode() == -1 || this->setSocketForLocal()){ // set socket for receving data.
        std::cerr << "Running socket is failed" << std::endl;
        return -1;
    }
    if (this->sendPingPongToNode() == -1 || this->sendPingPongToLocal() == -1){ // Node will play as hub.
        std::cerr << "send is failed" << std::endl;
        return -1;
    }
    std::string dataType;

    while (true){
        if (this->acceptSocket() == -1){ 
            std::cerr << "readySocket is failed" << std::endl;
        }
        // read header to know whether this message is from local or external and file or string.
        this->readHeader();
        dataType = this->receiveData();
        char* content = new char[strlen(this->buffer) + 1];
        strcpy(content, this->buffer);
        delete this->buffer;
        if (this->new_socket == this->local_socket){ // socket is from local. Therefore, send data to Hub
            sendDataToHub(content, dataType);
        }else{ // // socket is from Hub. Therefore, send data to Local
            sendDataToLocal(content, dataType);
        }
        delete content;
    }
    return 1;
}

int NodeWithHub::broadcastIpToNode(void){
    int sockfd;
    struct sockaddr_in broadcastAddr; // 브로드캐스트 주소 구조체

    if (this->nodeInfo.department == PreExamination){
        this->myIp += ",1";
    }else{
        this->myIp += ",2";
    }
    const char* msg = this->myIp.c_str();
    // UDP 소켓 생성
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    // 브로드캐스트를 활성화
    int broadcastEnable=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Error in setting Broadcast option");
        return -1;
    }
    // 브로드캐스트 주소 설정
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(8080); // 대상 포트
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255"); // 브로드캐스트 주소
    // 메시지 보내기
    if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        perror("sendto failed");
        return -1;
    }
    std::cout << "Broadcast message sent.\n";
    close(sockfd);
    return 0;
}

int NodeWithHub::readHeader(void){
    std::string client_ip_str;

    inet_ntop(AF_INET, &(this->local_addr.sin_addr), this->client_ip, INET_ADDRSTRLEN);
    std::cout << "Client connected from " << this->client_ip << ":" << ntohs(this->local_addr.sin_port) << std::endl;
    read(this->new_socket, this->header, 1);
    this->header[4] = '\0';

    // read length of data that client has sent. And then set the buffer.
    read(this->new_socket, &(this->dataLength), sizeof(this->dataLength));
    this->dataLength = ntohl(this->dataLength);
    this->buffer = new char[this->dataLength + 1];
    this->buffer[this->dataLength] = '\0';
    client_ip_str = this->client_ip;
    return 1;
}



int NodeWithHub::sendPingPongToNode(void){
    char ping[5] = "PING";
    char pong[5];

    if ((this->node_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    this->node_addr.sin_family = AF_INET;
    this->node_addr.sin_port = htons(HUBPORT);

    if (inet_pton(AF_INET, this->hubIp.c_str(), &(this->node_addr.sin_addr)) <= 0){
        std::cerr << "Invaild address / Address not supported" << std::endl;
        return -1;
    }
    if (connect(this->node_socket, (struct sockaddr *)&(this->node_addr), sizeof(this->node_addr)) < 0){
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }
    send(this->node_socket, ping, sizeof(ping), 0);
    std::cout << "Ping is sended to Hub" << std::endl;
    read(this->node_socket, pong, sizeof(pong));
    pong[4] = '\0';
    if (strcmp(pong, "PONG") == 0){
        std::cout << "Pong is sended from Hub";
    }else{
        std::cerr << "Hub: " << pong << ", connection failed" << std::endl;
        exit(1);
    }
    return 1;
}

int NodeWithHub::sendPingPongToLocal(void){
    const char* localhost = "127.0.0.1";
    char ping[5] = "PING";
    char pong[5];
    
    if (this->local_socket = socket(AF_INET, SOCK_STREAM, 0) < 0){
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    this->local_addr.sin_family = AF_INET;
    this->local_addr.sin_port = htons(LOCALPORT);
    if (inet_pton(AF_INET, localhost, &(this->local_addr.sin_addr)) <= 0){
        std::cerr << "Invaild address / Address not supported" << std::endl;
        return -1;
    }
    if (connect(this->local_socket, (struct sockaddr *)&(this->local_addr), sizeof(this->local_addr)) < 0){
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }
    send(this->local_socket, ping, sizeof(ping), 0);
    std::cout << "Ping is sended to Hub" << std::endl;
    read(this->local_socket, pong, sizeof(pong));
    pong[4] = '\0';
    if (strcmp(pong, "PONG") == 0){
        std::cout << "Pong is sended from Hub";
    }else{
        std::cerr << "Hub: " << pong << ", connection failed" << std::endl;
        exit(1);
    }
    return 1;

}

void NodeWithHub::sendDataToHub(const char* content, std::string &dataType){
    // 데이터 전송 로직 구현
    send(this->node_socket, dataType.c_str(), dataType.size(), 0);
    uint32_t dataLength = htonl(this->dataLength);
    send(this->node_socket, &dataLength, sizeof(dataLength), 0);
    send(this->node_socket, content, strlen(content), 0);
}

void NodeWithHub::sendDataToLocal(const char* content, std::string &dataType){
    // 데이터 전송 로직 구현
    send(this->local_socket, dataType.c_str(), dataType.size(), 0);
    uint32_t dataLength = htonl(this->dataLength);
    send(this->local_socket, &dataLength, sizeof(dataLength), 0);
    send(this->local_socket, content, strlen(content), 0);
}

std::string NodeWithHub::receiveData(void){
    // 데이터 수신 로직 구현
    int bytesReceived;
    // std::stringstream ss;
    std::string dataType;
    if (strcmp(this->header, "FILE") == 0){
        dataType = "FILE";
    }else if (strcmp(this->header, "TEXT") == 0){
        dataType = "TEXT";
    }
    std::cout << "File header is set" << std::endl;
    bytesReceived = read(this->local_socket, this->buffer, sizeof(buffer));
    if (bytesReceived != this->dataLength){
        std::cerr << "byteReceived: " << bytesReceived << "dataLength: " << this->dataLength << "which is not same" << std::endl;
    }
    // ss << this->buffer;
    // if (!ss.eof()){
    //     std::cout << "successively read File data" << std::endl;
    // }
    // std::string content = ss.str();
    // dataType = "FILE";
    // this->sendData(content, dataType);
    return dataType;
    //std::cout << "NodeWithHub received data: " << data << std::endl;
}

// void NodeWithHub::receiveDataFromExternal(void){

// }

// void NodeWithHub::sendTextData(std::string &string_content){
//     if (this->setSocketForSend() == -1){
//         std::cerr << "send is failed" << std::endl;
//         delete this->buffer;
//         exit(1);
//     }
//     std::string dataType = "TEXT";

// }