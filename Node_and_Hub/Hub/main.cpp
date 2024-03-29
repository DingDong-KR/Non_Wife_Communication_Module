#include "hub.hpp"

int main(int argc, char **argv, char **env){
    if (argc != 3) {
        std::cerr << "Argument number is not 2" << std::endl;
        return -1;
    }

    std::string Username = argv[1];
    int Dep = std::atoi(argv[2]);
    Department department = static_cast<Department>(Dep);

    Hub *hub = new Hub(Username, department);
    if (hub->run() == -1){
        std::cerr << "program failed" << std::endl;
        delete hub;
        return (-1);
    }
    delete hub;
    return (0);
}