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


	void handleRegistrationMessage(Client &client, const std::string &cmd);
	void commandRouter(Client *client, const std::string &cmd);
	void sendNonBlockingCommand(int fd, const std::string &message);


	int _port;
	int _serverSocketFd;
	std::string _password;
	static bool _signal;
	std::vector<struct pollfd> _pollFds;
	std::vector<Client> _clients;
	std::vector<Channel> _channels;
};

#endif
