#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include "Client.hpp"
#include <vector>

class Client;

class Channel {
public:
  Channel(std::string name);
  ~Channel();

  //! PUBLIC METHODS
  void addClient(Client *client);
  void removeClient(Client *client);

  void addOperator(Client *client);
  void removeOperator(Client *client);
  bool isOperator(Client *client);

  void addInvitedClient(Client *client);
  void removeInvitedClient(Client *client);
  bool isInvited(Client *client) const;

  void broadcast(const std::string &message, Client *exclude);

  //! SETTERS
  void setName(std::string name);
  void setTopic(std::string topic);
  void setMode(std::string modeStr, std::string param, Client *client);
  void setPassword(std::string password);
  void setMaxUsers(int maxUsers);
  void setClientCount(int clientCount);
  void setInviteOnly(bool inviteOnly);
  void setTopicProtected(bool topicProtected);
  void setPasswordProtected(bool passwordProtected);
  void setUserLimitEnabled(bool userLimitEnabled);

  //! GETTERS
  std::string getName();
  std::string getTopic();
  std::string getPassword();
  int getMaxUsers();
  std::vector<Client *> &getClients();
  std::vector<Client *> &getOperators();
  int getClientCount();
  bool isInviteOnly();
  bool isTopicProtected();
  bool isPasswordProtected();
  bool isUserLimitEnabled();

  //! DEBUG METHODS
  void debugPrintClients();
  void debugPrintOperators();
  void debugPrintInvitedClients();

private:
  std::string _name;
  std::string _password;
  std::string _topic;
  int _clientCount;
  int _maxUsers;
  bool _inviteOnly;
  bool _topicProtected;
  bool _passwordProtected;
  bool _userLimitEnabled;
  std::vector<Client *> _clients;
  std::vector<Client *> _invitedClients; // For +i mode
  std::vector<Client *> _operators;
};

#endif
