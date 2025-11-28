#include "../includes/Server.hpp"

bool Server::_signal = false;
// std::vector<Client> Server::_clients;

Server::Server() : _port(0), _serverSocketFd(-1), _password("") {
  std::cout << RED "Error default Server constructor called" << RESET
            << std::endl;
}

Server::Server(int port, std::string password)
    : _port(port), _password(password) {
  // std::cout << "Port: " << _port << std::endl;
  // std::cout << "Password: " << _password << std::endl;

  serverInit();
}

Server::~Server() {}

void Server::SignalHandler(int signum) {
  std::cout << BLUE << "Signal received: " << signum << RESET << std::endl;
  _signal = true;
}

void Server::serverInit() {
  std::cout << BOLDGREEN
            << "\n"
               "[" RESET BOLD "LOGS" BOLDGREEN "] "
            << RESET << "Initializing server..." << std::endl;
  createSocket();

  while (this->_signal == false) {
    int ret = poll(this->_pollFds.data(), this->_pollFds.size(), -1);
    if (ret == -1 && this->_signal == false) {
      throw std::runtime_error("poll() failed");
    }
    for (size_t i = 0; i < this->_pollFds.size(); i++) {
      if (this->_pollFds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
        if (this->_pollFds[i].fd == this->_serverSocketFd) {
          acceptClient();
        } else {
          Client *client = getClientByFd(this->_pollFds[i].fd);
          if (client != NULL) {
            handleMessage(this->_pollFds[i].fd, client);
          } else {
            std::cout << "Unknown client fd: " << this->_pollFds[i].fd
                      << std::endl;
          }
        }
      }
    }
  }
  closeSockets();
}

void Server::sendNonBlockingCommand(int fd, const std::string &message) {
  ssize_t totalSent = 0;
  ssize_t messageLength = message.length();
  const char *msgPtr = message.c_str();

  std::cout << BOLDBLUE "[" RESET BOLD "SEND" BOLDBLUE "] " RESET << message;

  while (totalSent < messageLength) {
    ssize_t bytesSent =
        send(fd, msgPtr + totalSent, messageLength - totalSent, 0);
    if (bytesSent == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Socket is not ready for sending, try again later
        continue;
      } else {
        std::cout << "send() failed in sendNonBlockingCommand: "
                  << strerror(errno) << std::endl;
        //! Exit program cleanly
        return;
      }
    }
    totalSent += bytesSent;
  }
}

void Server::clearClient(int fd) {
  // 1. Find the client pointer
  Client *clientToRemove = getClientByFd(fd);
  if (!clientToRemove)
    return;

  // 2. Remove this client from ALL channels
  for (size_t i = 0; i < this->_channels.size(); i++) {
    // You need a method in Channel to force remove a client by pointer
    // regardless of whether they sent a PART command
    this->_channels[i].removeClient(clientToRemove);
    this->_channels[i].removeOperator(clientToRemove);
    this->_channels[i].removeInvitedClient(clientToRemove);
  }

  // 3. Remove from pollfds
  for (size_t i = 0; i < this->_pollFds.size(); i++) {
    if (this->_pollFds[i].fd == fd) {
      this->_pollFds.erase(this->_pollFds.begin() + i);
      break;
    }
  }

  // 4. Remove from clients list
  this->_clients.erase(fd);
  close(fd);
}

// void Server::parseCommand(Client *client, const std::string &line)
// {
// 	// std::cout << GREEN << "parseCommand called" << RESET << std::endl;

// 	// std::cout << "[parseCommand] fd=" << client->getSocketFd() << "
// line=" << line << std::endl; 	(void)client;

// 	// (void)line;
// 	std::cout << line << std::endl;
// }

/* Checking before adding to a channel
1 check if +i mode is set and if the client is invited
2 check if +k mode is set and if the client provided the correct password
3 check if +l mode is set and if the channel is full
*/
bool Server::canJoinChannel(Channel &channel, Client *client,
                            const std::string &password) {
  // +i mode - Invite-only
  if (channel.isInviteOnly() && !channel.isInvited(client)) {
    std::string errorMsg = "473 " + client->getNickname() + " " +
                           channel.getName() + " :Cannot join channel (+i)\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return false;
  }

  // +k mode - Password protected
  if (!channel.getPassword().empty() && channel.getPassword() != password) {
    std::string errorMsg = "475 " + client->getNickname() + " " +
                           channel.getName() +
                           " :Cannot join channel (+k) - Bad key\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return false;
  }

  // +l mode - User limit
  if (channel.getMaxUsers() > 0 &&
      (int)channel.getClients().size() >= channel.getMaxUsers()) {
    std::string errorMsg = "471 " + client->getNickname() + " " +
                           channel.getName() +
                           " :Cannot join channel (+l) - Channel is full\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return false;
  }

  return true;
}

struct CommandHandler {
  std::string command;
  void (Server::*handler)(Client *, const std::string &);
};

void Server::handleJoin(Client *client, const std::string &cmd) {
  size_t pos = cmd.find(' ');
  if (pos == std::string::npos) {
    std::string errorMsg =
        "461 " + client->getNickname() + " JOIN :Not enough parameters\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  std::string channelName = cmd.substr(pos + 1);
  std::string password = "";
  size_t passPos = channelName.find(' ');
  if (passPos != std::string::npos) {
    password = channelName.substr(passPos + 1);
    channelName = channelName.substr(0, passPos);
  }

  // Check if channel already exists
  for (size_t i = 0; i < this->_channels.size(); i++) {
    if (this->_channels[i].getName() == channelName) {
      if (!canJoinChannel(this->_channels[i], client, password)) {
        return;
      }
      this->_channels[i].addClient(client);
      this->_channels[i].removeInvitedClient(
          client); // Remove from invited list upon joining
      std::string joinMsg = ":" + client->getNickname() + "!" +
                            client->getUsername() + "@" + client->getIp() +
                            " JOIN " + channelName + "\r\n";
      this->_channels[i].broadcast(
          joinMsg, NULL); // Broadcast to all including the joiner

      // Send names list to the joining client
      sendNamesList(client, &this->_channels[i]);
      return;
    }
  }

  // If channel does not exist, create a new one and add the client
  Channel newChannel(channelName);
  newChannel.addClient(client);
  newChannel.addOperator(client);
  this->_channels.push_back(newChannel);

  std::string joinMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@" + client->getIp() +
                        " JOIN " + channelName + "\r\n";
  this->_channels.back().broadcast(joinMsg, NULL);

  // Send names list to the joining client
  sendNamesList(client, &this->_channels.back());
}

void Server::handlePrivmsg(Client *client, const std::string &cmd) {
  size_t firstSpace = cmd.find(' ');
  if (firstSpace == std::string::npos)
    return;
  size_t secondSpace = cmd.find(' ', firstSpace + 1);
  if (secondSpace == std::string::npos)
    return;

  std::string target = cmd.substr(firstSpace + 1, secondSpace - firstSpace - 1);
  std::string message = cmd.substr(secondSpace + 1);

  // Remove leading ':' from message if present
  if (!message.empty() && message[0] == ':')
    message = message.substr(1);

  // Check if target is a channel (starts with '#')
  if (!target.empty() && target[0] == '#') {
    for (size_t i = 0; i < this->_channels.size(); i++) {
      if (this->_channels[i].getName() == target) {
        std::string privmsg = ":" + client->getNickname() + "!" +
                              client->getUsername() + "@" + client->getIp() +
                              " PRIVMSG " + target + " :" + message + "\r\n";
        this->_channels[i].broadcast(privmsg, client);
        return;
      }
    }
    // Channel not found
    std::string errorMsg =
        "401 " + client->getNickname() + " " + target + " :No such channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
  }
}

void Server::handlePart(Client *client, const std::string &cmd) {
  size_t pos = cmd.find(' ');
  if (pos == std::string::npos) {
    std::string errorMsg =
        "461 " + client->getNickname() + " PART :Not enough parameters\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  std::string channelName = cmd.substr(pos + 1);

  for (size_t i = 0; i < this->_channels.size(); i++) {
    if (this->_channels[i].getName() == channelName) {
      this->_channels[i].removeClient(client);
      this->_channels[i].removeOperator(client);
      this->_channels[i].removeInvitedClient(client);
      std::string partMsg = ":" + client->getNickname() + "!" +
                            client->getUsername() + "@" + client->getIp() +
                            " PART " + channelName + "\r\n";
      sendNonBlockingCommand(client->getSocketFd(), partMsg);

      if (this->_channels[i].getClients().empty()) {
        this->_channels.erase(this->_channels.begin() + i);
      }
      return;
    }
  }
  std::string errorMsg = "403 " + client->getNickname() + " " + channelName +
                         " :No such channel\r\n";
  sendNonBlockingCommand(client->getSocketFd(), errorMsg);
}

void Server::handleMode(Client *client, const std::string &cmd) {
  std::istringstream iss(cmd);
  std::string command, target;
  iss >> command >> target;

  // Check if the target is a channel
  if (!target.empty() && target[0] == '#') {
    // It is a channel mode (e.g., MODE #channel +o nick)
    // Delegate to your operator logic
    handleChannelMode(client, cmd);
  } else {
    (void)cmd; // To avoid unused parameter warning
    std::string reply = ":ircserv 324 " + client->getNickname() + " " +
                        client->getNickname() + " +i\r\n";

    sendNonBlockingCommand(client->getSocketFd(), reply);
  }
}

/* TOPIC
To VIEW - /topic #channel
To SET - /topic #channel New topic here -> SERVER RECEIVES: TOPIC #channel :New
topic here
*/
void Server::handleTopic(Client *client, const std::string &cmd) {
  std::istringstream iss(cmd);
  std::string command, channelName, topic;
  iss >> command >> channelName;

  size_t channelPos = cmd.find(channelName);
  size_t topicPos = std::string::npos;
  bool isSetting = false;

  if (channelPos != std::string::npos) {
    topicPos = cmd.find(" :", channelPos + channelName.length());
  }

  if (topicPos != std::string::npos) {
    topic = cmd.substr(topicPos + 2);
    isSetting = true;
  }

  Channel *channel = NULL;
  for (size_t i = 0; i < this->_channels.size(); i++) {
    if (this->_channels[i].getName() == channelName) {
      channel = &this->_channels[i];
      break;
    }
  }
  if (!channel) {
    std::string errorMsg = "403 " + client->getNickname() + " " + channelName +
                           " :No such channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  if (!isSetting) {
    if (channel->getTopic().empty()) {
      std::string reply = "331 " + client->getNickname() + " " + channelName +
                          " :No topic is set\r\n";
      sendNonBlockingCommand(client->getSocketFd(), reply);
    } else {
      std::string reply = "332 " + client->getNickname() + " " + channelName +
                          " :" + channel->getTopic() + "\r\n";
      sendNonBlockingCommand(client->getSocketFd(), reply);
    }
    return;
  }

  if (channel->isTopicProtected() && !channel->isOperator(client)) {
    std::string errorMsg = "482 " + client->getNickname() + " " + channelName +
                           " :You're not channel operator\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  if (!isSetting) {
    if (channel->getTopic().empty()) {
      std::string reply = "331 " + client->getNickname() + " " + channelName +
                          " :No topic is set\r\n";
      sendNonBlockingCommand(client->getSocketFd(), reply);
    } else {
      std::string reply = "332 " + client->getNickname() + " " + channelName +
                          " :" + channel->getTopic() + "\r\n";
      sendNonBlockingCommand(client->getSocketFd(), reply);
    }
    return;
  }

  if (channel->isTopicProtected() && !channel->isOperator(client)) {
    std::string errorMsg = "482 " + client->getNickname() + " " + channelName +
                           " :You're not channel operator\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  channel->setTopic(topic);
  std::string topicMsg = ":" + client->getNickname() + "!" +
                         client->getUsername() + "@" + client->getIp() +
                         " TOPIC " + channelName + " :" + topic + "\r\n";
  channel->broadcast(topicMsg, NULL);
}

void Server::handleInvite(Client *client, const std::string &cmd) {
  std::istringstream iss(cmd);
  std::string command, targetNick, channelName;
  iss >> command >> targetNick >> channelName;

  Channel *channel = NULL;
  for (size_t i = 0; i < this->_channels.size(); i++) {
    if (this->_channels[i].getName() == channelName) {
      channel = &this->_channels[i];
      break;
    }
  }
  if (!channel) {
    std::string errorMsg = "403 " + client->getNickname() + " " + channelName +
                           " :No such channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  if (!channel->isOperator(client)) {
    std::string errorMsg = "482 " + client->getNickname() + " " + channelName +
                           " :You're not channel operator\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  Client *targetClient = NULL;
  for (std::map<int, Client>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second.getNickname() == targetNick) {
      targetClient = &(it->second);
      break;
    }
  }
  if (!targetClient) {
    std::string errorMsg = "401 " + client->getNickname() + " " + targetNick +
                           " :No such nick/channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  channel->addInvitedClient(targetClient);
  std::string inviteMsg = ":" + client->getNickname() + "!" +
                          client->getUsername() + "@" + client->getIp() +
                          " INVITE " + targetNick + " " + channelName + "\r\n";
  sendNonBlockingCommand(targetClient->getSocketFd(), inviteMsg);
}

struct OperatorCommandHandler {
  std::string command;
  void (Server::*handler)(Client *, const std::string &);
};

static OperatorCommandHandler operatorHandlers[] = {
    {"KICK", &Server::handleKick},
    {"INVITE", &Server::handleInvite},
    {"TOPIC", &Server::handleTopic},
    {"MODE", &Server::handleChannelMode}, // For channel modes only
};

void Server::operatorCommandRouter(Client *client, const std::string &cmd) {
  for (size_t i = 0; i < sizeof(operatorHandlers) / sizeof(operatorHandlers[0]);
       i++) {
    if (strncmp(cmd.c_str(), operatorHandlers[i].command.c_str(),
                operatorHandlers[i].command.length()) == 0) {
      (this->*operatorHandlers[i].handler)(client, cmd);
      return;
    }
  }
  std::string errorMsg = "421 " + client->getNickname() + " " +
                         cmd.substr(0, cmd.find(' ')) + " :Unknown command\r\n";
  sendNonBlockingCommand(client->getSocketFd(), errorMsg);
}

void Server::handleKick(Client *client, const std::string &cmd) {
  std::istringstream iss(cmd);
  std::string command, channelName, targetNick, reason;
  iss >> command >> channelName >> targetNick;
  std::getline(iss, reason);
  if (!reason.empty() && reason[0] == ' ') {
    reason = reason.substr(1);
  }

  Channel *channel = NULL;
  for (size_t i = 0; i < this->_channels.size(); i++) {
    if (this->_channels[i].getName() == channelName) {
      channel = &this->_channels[i];
      break;
    }
  }
  if (!channel) {
    std::string errorMsg = "403 " + client->getNickname() + " " + channelName +
                           " :No such channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  if (!channel->isOperator(client)) {
    std::string errorMsg = "482 " + client->getNickname() + " " + channelName +
                           " :You're not channel operator\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  Client *targetClient = NULL;
  std::vector<Client *> channelClients = channel->getClients();
  for (size_t i = 0; i < channelClients.size(); i++) {
    if (channelClients[i]->getNickname() == targetNick) {
      targetClient = channelClients[i];
      break;
    }
  }
  if (!targetClient) {
    std::string errorMsg = "441 " + client->getNickname() + " " + targetNick +
                           " " + channelName +
                           " :They aren't on that channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  std::string kickMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@" + client->getIp() +
                        " KICK " + channel->getName() + " " +
                        targetClient->getNickname() + " :" + reason + "\r\n";
  sendNonBlockingCommand(targetClient->getSocketFd(), kickMsg);
  channel->removeClient(targetClient);
  channel->broadcast(kickMsg, NULL);
}

void Server::handleChannelMode(Client *client, const std::string &cmd) {
  std::istringstream iss(cmd);
  std::string command, channelName, modeStr, param;
  iss >> command >> channelName >> modeStr;
  if (modeStr.find("+o") == 0 || modeStr.find("-o") == 0) {
    iss >> param; // nick
  } else if (modeStr.find("+k") == 0) {
    iss >> param; // password
  } else if (modeStr.find("+l") == 0) {
    iss >> param; // limit
  }

  Channel *channel = NULL;
  for (size_t i = 0; i < this->_channels.size(); i++) {
    if (this->_channels[i].getName() == channelName) {
      channel = &this->_channels[i];
      break;
    }
  }
  if (!channel) {
    std::string errorMsg = "403 " + client->getNickname() + " " + channelName +
                           " :No such channel\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  if (modeStr.empty()) {
    std::string modes = "+";
    std::string args = "";

    if (channel->isInviteOnly())
      modes += "i";
    if (channel->isTopicProtected())
      modes += "t";
    if (!channel->getPassword().empty()) {
      modes += "k";
      args += " " + channel->getPassword();
    }
    if (channel->getMaxUsers() > 0) {
      modes += "l";
      std::ostringstream oss; // requires #include <sstream>
      oss << " " << channel->getMaxUsers();
      args += oss.str();
    }

    // 324 RPL_CHANNELMODEIS
    std::string reply = ":ircserv 324 " + client->getNickname() + " " +
                        channelName + " " + modes + args + "\r\n";
    sendNonBlockingCommand(client->getSocketFd(), reply);
    return;
  }

  // Listing ban mask(s): MODE #chan b
  if (modeStr == "b") {
    // Empty list: just send RPL_ENDOFBANLIST
    std::string end = "368 " + client->getNickname() + " " + channelName +
                      " :End of channel ban list\r\n";
    sendNonBlockingCommand(client->getSocketFd(), end);
    return;
  }

  // For actual changes, require operator
  if (!channel->isOperator(client)) {
    std::string errorMsg = "482 " + client->getNickname() + " " + channelName +
                           " :You're not channel operator\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }

  // Fix: Validate that the target user exists in the channel for +o/-o
  if (modeStr == "+o" || modeStr == "-o") {
    if (param.empty())
      return; // Ignore if no nickname provided

    bool found = false;
    std::vector<Client *> chClients = channel->getClients();
    for (size_t i = 0; i < chClients.size(); i++) {
      if (chClients[i]->getNickname() == param) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::string errorMsg = "441 " + client->getNickname() + " " + param +
                             " " + channelName +
                             " :They aren't on that channel\r\n";
      sendNonBlockingCommand(client->getSocketFd(), errorMsg);
      return;
    }
  }
  channel->setMode(modeStr, param, client);

  std::string modeMsg = ":" + client->getNickname() + "!" +
                        client->getUsername() + "@" + client->getIp() +
                        " MODE " + channelName + " " + modeStr;
  if (!param.empty()) {
    modeMsg += " " + param;
  }
  modeMsg += "\r\n";
  channel->broadcast(modeMsg, NULL);
}

void Server::handlePing(Client *client, const std::string &cmd) {
  size_t pos = cmd.find(' ');
  std::string param = (pos != std::string::npos) ? cmd.substr(pos + 1) : "";
  std::string pongMsg = "PONG " + param + "\r\n";
  sendNonBlockingCommand(client->getSocketFd(), pongMsg);
}

void Server::commandRouter(Client *client, const std::string &cmd) {
  if (cmd.empty())
    return;

  static CommandHandler handlers[] = {
      {"JOIN", &Server::handleJoin},
      {"PRIVMSG", &Server::handlePrivmsg},
      {"PART", &Server::handlePart},
      //? Optionnal to silently ignore the commands we dont need to handle
      {"MODE", &Server::handleMode}, // Handles users and Channel modes
      {"PING", &Server::handlePing},
      //! Operator commands
      {"KICK", &Server::handleKick},
      {"INVITE", &Server::handleInvite},
      {"TOPIC", &Server::handleTopic},
      {"WHO", &Server::handleWho},
  };

  for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
    if (strncmp(cmd.c_str(), handlers[i].command.c_str(),
                handlers[i].command.length()) == 0) {
      (this->*handlers[i].handler)(client, cmd);
      return;
    }
  }

  std::string errorMsg = "421 " + client->getNickname() + " " +
                         cmd.substr(0, cmd.find(' ')) + " :Unknown command\r\n";
  sendNonBlockingCommand(client->getSocketFd(), errorMsg);
  return;
}

void Server::sendNamesList(Client *client, Channel *channel) {
  std::string namesList = "";
  std::vector<Client *> clients = channel->getClients();

  for (size_t i = 0; i < clients.size(); i++) {
    // Add '@' prefix for operators
    if (channel->isOperator(clients[i])) {
      namesList += "@";
    }
    namesList += clients[i]->getNickname();

    // Add space between names (not after the last one)
    if (i < clients.size() - 1) {
      namesList += " ";
    }
  }

  // 353 RPL_NAMREPLY
  std::string reply = ":ircserv 353 " + client->getNickname() + " = " +
                      channel->getName() + " :" + namesList + "\r\n";
  sendNonBlockingCommand(client->getSocketFd(), reply);

  // 366 RPL_ENDOFNAMES
  std::string endReply = ":ircserv 366 " + client->getNickname() + " " +
                         channel->getName() + " :End of /NAMES list\r\n";
  sendNonBlockingCommand(client->getSocketFd(), endReply);
}

void Server::handleWho(Client *client, const std::string &cmd) {
  std::istringstream iss(cmd);
  std::string command, mask;
  iss >> command >> mask;
  if (mask.empty()) {
    std::string end =
        ":ircserv 315 " + client->getNickname() + " * :End of WHO list\r\n";
    sendNonBlockingCommand(client->getSocketFd(), end);
    return;
  }
  if (!mask.empty() && mask[0] == '#') {
    Channel *channel = NULL;
    for (size_t i = 0; i < this->_channels.size(); i++) {
      if (this->_channels[i].getName() == mask) {
        channel = &this->_channels[i];
        break;
      }
    }
    if (channel) {
      std::vector<Client *> chClients = channel->getClients();
      for (size_t i = 0; i < chClients.size(); i++) {
        Client *c = chClients[i];
        std::string flags = "H";
        if (channel->isOperator(c)) {
          flags += "@";
        }
        std::string line = ":ircserv 352 " + client->getNickname() + " " +
                           mask + " " + c->getUsername() + " " + c->getIp() +
                           " ircserv " + c->getNickname() + " " + flags +
                           " :0 " + c->getUsername() + "\r\n";
        sendNonBlockingCommand(client->getSocketFd(), line);
      }
    }
    std::string end = ":ircserv 315 " + client->getNickname() + " " + mask +
                      " :End of WHO list\r\n";
    sendNonBlockingCommand(client->getSocketFd(), end);
    return;
  }
  for (std::map<int, Client>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    Client &c = it->second;
    if (c.getNickname() == mask) {
      std::string line = ":ircserv 352 " + client->getNickname() + " * " +
                         c.getUsername() + " " + c.getIp() + " ircserv " +
                         c.getNickname() + " H :0 " + c.getUsername() + "\r\n";
      sendNonBlockingCommand(client->getSocketFd(), line);
      break;
    }
  }
  std::string end = ":ircserv 315 " + client->getNickname() + " " + mask +
                    " :End of WHO list\r\n";
  sendNonBlockingCommand(client->getSocketFd(), end);
}
void Server::handleMessage(int fd, Client *client) {
  std::cout << BOLDYELLOW "\n["
            << RESET BOLD "RECV" BOLDYELLOW "] ----------------" << RESET
            << std::endl;
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);

  if (bytesReceived <= 0) {
    if (bytesReceived == 0) {
      std::cout << "Client " << fd << " disconnected" << std::endl;
      clearClient(fd);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cout << "recv() error on client " << fd << ": " << strerror(errno)
                << std::endl;
      clearClient(fd);
    }
    return;
  }
  if (bytesReceived > 512) {
    std::cout << "Message is too long" << std::endl;
    return;
  }

  buffer[bytesReceived] = '\0';

  client->setReadbuffer(client->getReadbuffer() + buffer);

  while (1) {
    size_t pos = client->getReadbuffer().find("\r\n");
    if (pos == std::string::npos) {
      break;
    }
    std::string command = client->getReadbuffer().substr(0, pos);
    client->setReadbuffer(client->getReadbuffer().substr(pos + 2));
    std::cout << BOLDMAGENTA "[" << RESET BOLD << "CMDS" << BOLDMAGENTA "] "
              << RESET << command << std::endl;
    if (!client->getAuthenticated()) {
      handleRegistrationMessage(*client, command);
      if (_clients.find(fd) == _clients.end()) {
        return;
      }
    } else {
      commandRouter(client, command);
    }
  }
}

void Server::handleRegistrationMessage(Client &client, const std::string &cmd) {
  if (strncmp(cmd.c_str(), "PASS", 4) == 0 && !client.getHasPass()) {
    handlePass(&client, cmd);
  } else if (strncmp(cmd.c_str(), "NICK", 4) == 0 && !client.getHasNick()) {
    handleNick(&client, cmd);
  } else if (strncmp(cmd.c_str(), "USER", 4) == 0 && !client.getHasUser()) {
    handleUser(&client, cmd);
  } else if (strncmp(cmd.c_str(), "CAP", 3) == 0) {
    handleCap(&client, cmd);
  }

  if (client.getHasPass() && client.getHasNick() && client.getHasUser()) {
    for (std::map<int, Client>::iterator it = this->_clients.begin();
         it != this->_clients.end(); ++it) {
      if (&(it->second) != &client &&
          it->second.getNickname() == client.getNickname() &&
          it->second.getAuthenticated()) {
        std::string errorMsg =
            "433 " + client.getNickname() + " :Nickname is already in use\r\n";
        sendNonBlockingCommand(client.getSocketFd(), errorMsg);
        clearClient(client.getSocketFd());
        return;
      }
    }
    client.setAuthenticated(true);
    std::string welcomeMsg = "001 " + client.getNickname() +
                             " :" BOLDGREEN "Welcome to the IRC server" RESET
                             "\r\n";
    sendNonBlockingCommand(client.getSocketFd(), welcomeMsg);
  }
}

void Server::handleCap(Client *client, const std::string &cmd) {
  const std::string target = client->getHasNick() ? client->getNickname() : "*";
  if (cmd.rfind("CAP END", 0) == 0) {
    return;
  } else if (cmd.rfind("CAP LS", 0) == 0) {
    std::string response = ":ircserv CAP " + target + " LS :\r\n";
    sendNonBlockingCommand(client->getSocketFd(), response);
  } else if (cmd.rfind("CAP REQ", 0) == 0) {
    std::string response = ":ircserv CAP " + target + " ACK :\r\n";
    sendNonBlockingCommand(client->getSocketFd(), response);
  } else if (cmd.rfind("CAP LIST", 0) == 0) {
    std::string response = ":ircserv CAP " + target + " LIST :\r\n";
    sendNonBlockingCommand(client->getSocketFd(), response);
  }
}

// void Server::handleCap(Client *client, const std::string &cmd) {
//   const std::string target = client->getHasNick() ? client->getNickname() :
//   "*"; if (cmd == "CAP LS" || cmd.find("CAP LS") == 0) {
//     // No capabilities to advertise for now
//     std::string response = ":ircserv CAP " + target + " LS :\r\n";
//     sendNonBlockingCommand(client->getSocketFd(), response);
//   } else if (cmd.rfind("CAP REQ", 0) == 0) {
//     // Minimal ACK; you can parse and selectively ACK/NAK later
//     std::string response = ":ircserv CAP " + target + " ACK :\r\n";
//     sendNonBlockingCommand(client->getSocketFd(), response);
//   } else {
//     std::string response = ":ircserv CAP " + target + " ACK :\r\n";
//     sendNonBlockingCommand(client->getSocketFd(), response);
//   }
// }

// void Server::handleCap(Client *client, const std::string &cmd) {
//   if (cmd == "CAP LS" || cmd.find("CAP LS") == 0) {
//     std::string response =
//         ": ircserv CAP " + client->getNickname() + " LS :\r\n";
//     sendNonBlockingCommand(client->getSocketFd(), response);
//   } else {
//     std::string response = ":ircserv CAP * ACK :capabilities
//     acknowledged\r\n"; sendNonBlockingCommand(client->getSocketFd(),
//     response);
//   }
// }

void Server::handlePass(Client *client, const std::string &cmd) {
  // Extract the password from the command
  size_t pos = cmd.find(' ');
  if (pos == std::string::npos) {
    std::string errorMsg =
        "461 " + client->getNickname() + " PASS :Not enough parameters\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  std::string password = cmd.substr(pos + 1);
  // Here you would check the password against the server's expected password
  int fd = client->getSocketFd();
  if (this->_password == password) {
    client->setHasPass(true);
  } else {
    std::string errorMsg =
        "464 " + client->getNickname() + " :Password incorrect\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    clearClient(fd);
    return;
  }
}

void Server::handleNick(Client *client, const std::string &cmd) {
  size_t pos = cmd.find(' ');
  if (pos == std::string::npos) {
    std::string errorMsg = "431 :No nickname given\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  std::string nickname = cmd.substr(pos + 1);

  // Check if the nickname is already in use (don't check authenticated status)
  int fd = client->getSocketFd();
  for (std::map<int, Client>::iterator it = this->_clients.begin();
       it != this->_clients.end(); ++it) {
    if (&(it->second) != client && it->second.getNickname() == nickname) {
      std::string errorMsg =
          "433 " + nickname + " :Nickname is already in use\r\n";
      sendNonBlockingCommand(client->getSocketFd(), errorMsg);
      clearClient(fd);
      return;
    }
  }
  client->setNickname(nickname);
  client->setHasNick(true);
}

void Server::handleUser(Client *client, const std::string &cmd) {
  size_t pos = cmd.find(' ');
  if (pos == std::string::npos) {
    std::string errorMsg =
        "461 " + client->getNickname() + " USER :Not enough parameters\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  std::string username = cmd.substr(pos + 1);
  // Divide into 4 parts: username, hostname, servername, realname and do the
  // checks for real name must start with ':' split by space and check if there
  // are at least 4 parts
  std::vector<std::string> parts;
  size_t start = 0;
  while (start < username.length()) {
    size_t end = username.find(' ', start);
    if (end == std::string::npos) {
      parts.push_back(username.substr(start));
      break;
    }
    parts.push_back(username.substr(start, end - start));
    start = end + 1;
  }
  if (parts.size() < 4) {
    std::string errorMsg =
        "461 " + client->getNickname() + " USER :Not enough parameters\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  // check the real name starts with ':'
  if (parts[3][0] != ':') {
    std::string errorMsg =
        "461 " + client->getNickname() + " USER :Not enough parameters\r\n";
    sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    return;
  }
  client->setUsername(parts[0]);
  client->setHasUser(true);
}

Client *Server::getClientByFd(int fd) {
  std::map<int, Client>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    return &(it->second);
  }
  return NULL;
}

void Server::acceptClient() {

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);

  int clientFd =
      accept(_serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
  if (clientFd == -1) {
    throw std::runtime_error("Failed to accept client: accept() failed");
  }

  Client cli(clientFd);
  cli.setIp(inet_ntoa(clientAddr.sin_addr));
  if (cli.getIp() == "") {
    throw std::runtime_error("Failed to get client IP: inet_ntoa() failed");
  }

  if (fcntl(cli.getSocketFd(), F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error(
        "Failed to set socket option to non-blocking: fcntl() failed");
  }

  struct pollfd clientPollFd;
  clientPollFd.fd = cli.getSocketFd();
  clientPollFd.events = POLLIN;
  clientPollFd.revents = 0;

  this->_pollFds.push_back(clientPollFd);
  this->_clients.insert(std::make_pair(cli.getSocketFd(), cli));
  std::cout << BOLDGREEN << "\n[" RESET BOLD "LOGS" BOLDGREEN "] " << RESET
            << "Client accepted fd=" << cli.getSocketFd() << std::endl;
}

void Server::closeSockets() {
  std::cout << BOLDGREEN << "[" RESET BOLD "LOGS" BOLDGREEN "] " << RESET
            << "Closing socket connections" << std::endl;
  for (size_t i = 0; i < this->_pollFds.size(); i++) {
    close(this->_pollFds[i].fd);
  }
  this->_pollFds.clear();
}

void Server::createSocket() {
  std::cout << BOLDGREEN << "[" RESET BOLD "LOGS" BOLDGREEN "] " << RESET
            << "Creating socket on port " << this->_port << std::endl;
  int en = 1;
  struct sockaddr_in add;
  struct pollfd serverPollFd;

  add.sin_family = AF_INET;
  add.sin_addr.s_addr = INADDR_ANY;
  add.sin_port = htons(this->_port);

  this->_serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->_serverSocketFd == -1) {
    throw std::runtime_error("Failed to create socket: socket() failed");
  }

  if (setsockopt(this->_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &en,
                 sizeof(en)) == -1) {
    throw std::runtime_error(
        "Failed to set socket options: setsockopt() failed");
  }

  if (fcntl(this->_serverSocketFd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error(
        "Failed to set socket option to non-blocking: fcntl() failed");
  }

  if (bind(this->_serverSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1) {
    throw std::runtime_error("Failed to bind socket: bind() failed");
  }

  if (listen(this->_serverSocketFd, SOMAXCONN) == -1) {
    throw std::runtime_error("Failed to listen on socket: listen() failed");
  }

  //! Re explain this
  serverPollFd.fd = this->_serverSocketFd;
  serverPollFd.events = POLLIN;
  serverPollFd.revents = 0;

  this->_pollFds.push_back(serverPollFd);
  std::cout << BOLDGREEN "\nServer listening on 0.0.0.0:" << this->_port
            << RESET "\n"
            << std::endl;
}
