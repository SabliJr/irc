#include "../includes/Utils.hpp"
#include "../includes/Channel.hpp"

Channel::Channel(std::string name) : _name(name), _password(""), _topic(""),  _clientCount(1), _maxUsers(10),
_clients(std::vector<Client *>()), _operators(std::vector<Client *>()) {}

Channel::~Channel() {}

//! GETTERS
std::string Channel::getName() {
    return this->_name;
}

std::string Channel::getTopic() {
    return this->_topic;
}

std::string Channel::getPassword() {
    return this->_password;
}

int Channel::getMaxUsers() {
    return this->_maxUsers;
}

std::vector <Client *> Channel::getClients() {
    return this->_clients;
}

std::vector <Client *> Channel::getOperators() {
    return this->_operators;
}

int Channel::getClientCount() {
    return this->_clientCount;
}

//! SETTERS
void Channel::setName(std::string name) {
    this->_name = name;
}

void Channel::setTopic(std::string topic) {
    this->_topic = topic;
}

void Channel::setPassword(std::string password) {
    this->_password = password;
}

void Channel::setMaxUsers(int maxUsers) {
    this->_maxUsers = maxUsers;
}

void Channel::setClientCount(int clientCount) {
    this->_clientCount = clientCount;
}

//! PUBLIC METHODS
void Channel::addClient(Client *client) {
    this->_clients.push_back(client);
    this->_clientCount++;
}

void Channel::removeClient(Client *client) {
    for (size_t i = 0; i < this->_clients.size(); i++) {
        if (this->_clients[i] == client) {
            this->_clients.erase(this->_clients.begin() + i);
            break;
        }
    }
    this->_clientCount--;
}

void Channel::addOperator(Client *client) {
    this->_operators.push_back(client);
}

void Channel::removeOperator(Client *client) {
    for (size_t i = 0; i < this->_operators.size(); i++) {
        if (this->_operators[i] == client) {
            this->_operators.erase(this->_operators.begin() + i);
            break;
        }
    }
}

void Channel::broadcast(std::string message) {
    for (size_t i = 0; i < this->_clients.size(); i++) {
        int res = send(this->_clients[i]->getSocketFd(), message.c_str(), message.length(), 0);
        if (res == -1) {
            std::cout << "Failed to broadcast message - send() failed in Channel::broadcast: " << strerror(errno) << std::endl;
        }
    }
}
