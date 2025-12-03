#include "../includes/Channel.hpp"
#include "../includes/Utils.hpp"

Channel::Channel(std::string name)
    : _name(name), _password(""), _topic(""), _clientCount(1), _maxUsers(10),
      _inviteOnly(false), _topicProtected(false),
      _clients(std::vector<Client *>()), _operators(std::vector<Client *>()) {}

Channel::~Channel() {}

//! GETTERS
std::string Channel::getName() { return this->_name; }

std::string Channel::getTopic() { return this->_topic; }

std::string Channel::getPassword() { return this->_password; }

int Channel::getMaxUsers() { return this->_maxUsers; }

std::vector<Client *> &Channel::getClients() { return this->_clients; }

std::vector<Client *> &Channel::getOperators() { return this->_operators; }

int Channel::getClientCount() { return this->_clientCount; }

bool Channel::isInviteOnly() { return this->_inviteOnly; }

bool Channel::isTopicProtected() { return this->_topicProtected; }

bool Channel::isPasswordProtected() { return this->_passwordProtected; }

bool Channel::isUserLimitEnabled() { return this->_userLimitEnabled; }

//! SETTERS
void Channel::setName(std::string name) { this->_name = name; }

void Channel::setTopic(std::string topic) { this->_topic = topic; }

void Channel::setPassword(std::string password) { this->_password = password; }

void Channel::setMaxUsers(int maxUsers) { this->_maxUsers = maxUsers; }

void Channel::setClientCount(int clientCount) {
  this->_clientCount = clientCount;
}

void Channel::setPasswordProtected(bool passwordProtected) {
  this->_passwordProtected = passwordProtected;
}
void Channel::setInviteOnly(bool inviteOnly) { this->_inviteOnly = inviteOnly; }

void Channel::setTopicProtected(bool topicProtected) {
  this->_topicProtected = topicProtected;
}

void Channel::setUserLimitEnabled(bool userLimitEnabled) {
  this->_userLimitEnabled = userLimitEnabled;
}

void Channel::setMode(std::string modeStr, std::string param, Client *client) {
  if (modeStr == "+i") {
    this->setInviteOnly(true);
  } else if (modeStr == "-i") {
    this->setInviteOnly(false);
  } else if (modeStr == "+t") {
    this->setTopicProtected(true);
  } else if (modeStr == "-t") {
    this->setTopicProtected(false);
  } else if (modeStr == "+o") {
    (void)client;
    for (size_t i = 0; i < _clients.size(); i++) {
      if (_clients[i]->getNickname() == param) {
        this->addOperator(_clients[i]);
        return;
      }
    }
  } else if (modeStr == "-o") {
    (void)client;
    // FIX: Find the client named 'param' and remove THEM
    for (size_t i = 0; i < _clients.size(); i++) {
      if (_clients[i]->getNickname() == param) {
        this->removeOperator(_clients[i]);
        return;
      }
    }
  } else if (modeStr == "+k") {
    // Set password
    this->setPassword(param);
  } else if (modeStr == "-k") {
    // Unset password
    this->setPassword("");
  } else if (modeStr == "+l") {
    // Set user limit
    this->setMaxUsers(std::atoi(param.c_str()));
  } else if (modeStr == "-l") {
    // Unset user limit
    this->setMaxUsers(0); // 0 means no limit
  }
}

//! PUBLIC METHODS
void Channel::addClient(Client *client) {
  std::cout << "Adding client to channel: " << this->_name << std::endl;
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
  client->removeChannel(this->_name);
}

void Channel::addInvitedClient(Client *client) {
  this->_invitedClients.push_back(client);
}

bool Channel::isInvited(Client *client) const {
  for (size_t i = 0; i < this->_invitedClients.size(); i++) {
    if (this->_invitedClients[i] == client) {
      return true;
    }
  }
  return false;
}

void Channel::removeInvitedClient(Client *client) {
  for (size_t i = 0; i < this->_invitedClients.size(); i++) {
    if (this->_invitedClients[i] == client) {
      this->_invitedClients.erase(this->_invitedClients.begin() + i);
      break;
    }
  }
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

bool Channel::isOperator(Client *client) {
  for (size_t i = 0; i < this->_operators.size(); i++) {
    if (this->_operators[i] == client) {
      return true;
    }
  }
  return false;
}

void Channel::broadcast(const std::string &message, Client *exclude) {
  for (size_t i = 0; i < this->_clients.size(); i++) {
    if (this->_clients[i] != exclude) // Don't send to excluded client
    {
      ssize_t bytesSent = send(this->_clients[i]->getSocketFd(),
                               message.c_str(), message.length(), 0);
      if (bytesSent == -1) {
        std::cout << "Failed to broadcast message" << std::endl;
      }
    }
  }
}

// DEBUG  METHODS
void Channel::debugPrintClients() {
  std::cout << "Clients in channel: " << this->_name << std::endl;
  for (size_t i = 0; i < this->_clients.size(); i++) {
    std::cout << this->_clients[i]->getNickname() << std::endl;
  }
}

void Channel::debugPrintOperators() {
  std::cout << "Operators in channel: " << this->_name << std::endl;
  for (size_t i = 0; i < this->_operators.size(); i++) {
    std::cout << this->_operators[i]->getNickname() << std::endl;
  }
}

void Channel::debugPrintInvitedClients() {
  std::cout << "Invited clients in channel: " << this->_name << std::endl;
  for (size_t i = 0; i < this->_invitedClients.size(); i++) {
    std::cout << this->_invitedClients[i]->getNickname() << std::endl;
  }
}
