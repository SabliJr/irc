#include "Utils.hpp"

#ifndef CLIENT_HPP
#include "Client.hpp"
#endif

class Client;

class Server {
	public:
		Server(int port, std::string password);
		~Server();

		static void SignalHandler(int signum);

		void handleCAP(Client *client, const std::vector<std::string> &args);
		void handleNICK(Client *client, const std::vector<std::string> &args);
		void handlePASS(Client *client, const std::vector<std::string> &args);
		void handleUSER(Client *client, const std::vector<std::string> &args);



	private:
		int _port;
		int _serverSocketFd;
		std::string _password;
		static bool _signal;
		std::vector<struct pollfd> _pollFds;
		static std::vector<Client> _clients;

		Server();

		void ServerInit();
		void CreateSocket();
		void acceptClient();
		void handleMessage(int fd, Client *client);
		void CloseSockets();
		void ClearClients(int fd);
		void parseCommand(Client *client, const std::string &line);
		Client *getClientByFd(int fd);

};
