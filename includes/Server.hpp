#ifndef CLIENT_HPP
#include "Client.hpp"
#include "Utils.hpp"

class Client;

class Server
{
public:
	Server(int port, std::string password);
	~Server();

	static void SignalHandler(int signum);

private:
	Server();

	void ServerInit();
	void CreateSocket();
	void acceptClient();
	void handleMessage(int fd, Client *client);
	void CloseSockets();
	void ClearClients(int fd);
	// void parseCommand(Client *client, const std::string &line);
	Client *getClientByFd(int fd);

	void handlePass(Client *client, const std::string cmd);
	void handleNick(Client *client, const std::string cmd);
	void handleUser(Client *client, const std::string cmd);
	void handleCap(Client *client, const std::string cmd);


	void commandRouter(Client *client, const std::string cmd);

	int _port;
	int _serverSocketFd;
	std::string _password;
	static bool _signal;
	std::vector<struct pollfd> _pollFds;
	std::vector<Client> _clients;
};

#endif
