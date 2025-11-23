#ifndef CLIENT_HPP
#include "Client.hpp"
#include "Channel.hpp"
#include "Utils.hpp"

class Client;
class Channel;

class Server
{
public:
	Server(int port, std::string password);
	~Server();

	static void SignalHandler(int signum);
		//! Operator commands
	void handleKick(Client *client, const std::string &cmd);
	void handleInvite(Client *client, const std::string &cmd);
	void handleTopic(Client *client, const std::string &cmd);
	void handleChannelMode(Client *client, const std::string &cmd);

private:
	Server();

	void serverInit();
	void createSocket();
	void acceptClient();
	void handleMessage(int fd, Client *client);
	void closeSockets();
	void clearClients(int fd);
	// void parseCommand(Client *client, const std::string &line);
	Client *getClientByFd(int fd);

	void handlePass(Client *client, const std::string &cmd);
	void handleNick(Client *client, const std::string &cmd);
	void handleUser(Client *client, const std::string &cmd);
	void handleCap(Client *client, const std::string &cmd);

	void handleJoin(Client *client, const std::string &cmd);
	void handlePrivmsg(Client *client, const std::string &cmd);
	void handlePart(Client *client, const std::string &cmd);

	//! Command we dont need to handle for the project but to silently ignore them
	void handleMode(Client *client, const std::string &cmd);
	void handlePing(Client *client, const std::string &cmd);

	void operatorCommandRouter(Client *client, const std::string &cmd);


	void handleRegistrationMessage(Client &client, const std::string &cmd);
	void commandRouter(Client *client, const std::string &cmd);
	void sendNonBlockingCommand(int fd, const std::string &message);
	void sendNamesList(Client *client, Channel *channel);
	bool canJoinChannel(Channel &channel, Client *client, const std::string &password);



	int _port;
	int _serverSocketFd;
	std::string _password;
	static bool _signal;
	std::vector<struct pollfd> _pollFds;
	std::map<int, Client> _clients;
	std::vector<Channel> _channels;
};

#endif
