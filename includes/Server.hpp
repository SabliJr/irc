#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <vector>
#include <signal.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>


class Server {
	public:
		Server(int port, std::string password);
		~Server();

		static void SignalHandler(int signum);

	private:
		int _port;
		int _socketFd;
		std::string _password;
		static bool _signal;
		std::vector<struct pollfd> _pollFds;
		// static std::vector<Client> _clients;

		Server();

		void ServerInit();
		void CreateSocket();

};
