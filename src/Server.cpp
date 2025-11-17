#include "../includes/Server.hpp"

Server::Server() : _port(0), _socketFd(-1), _password("") {
	//! REMOVE THE BS
	std::cout << "Server constructor called" << std::endl;
	std::cout << "Port: " << _port << std::endl;
	std::cout << "Password: " << _password << std::endl;
}

Server::Server(int port, std::string password) : _port(port), _password(password) {
	std::cout << "Port: " << _port << std::endl;
	std::cout << "Password: " << _password << std::endl;

	ServerInit();
}

Server::~Server() {}

void Server::SignalHandler(int signum) {
	std::cout << "Signal received: " << signum << std::endl;
	std::cout << "DO THE NECESSARY CLEANUP" << std::endl;
	exit(0);
}

void Server::ServerInit() {
	std::cout << "ServerInit called" << std::endl;
	CreateSocket();
}

void Server::CreateSocket() {
	std::cout << "CreateSocket called" << std::endl;
	int en = 1;
	struct sockaddr_in add;
	struct pollfd serverPollFd;

	add.sin_family = AF_INET;
	add.sin_addr.s_addr = INADDR_ANY;
	add.sin_port = htons(this->_port);

	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd == -1) {
		throw std::runtime_error("Failed to create socket: socket() failed");
	}

	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) {
		throw std::runtime_error("Failed to set socket options: setsockopt() failed");
	}

	if (fcntl(_socketFd, F_SETFL, O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set socket option to non-blocking: fcntl() failed");
	}

	if (bind(_socketFd, (struct sockaddr *)&add, sizeof(add)) == -1) {
		throw std::runtime_error("Failed to bind socket: bind() failed");
	}

	if (listen(_socketFd, SOMAXCONN) == -1) {
		throw std::runtime_error("Failed to listen on socket: listen() failed");
	}

	serverPollFd.fd = _socketFd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	_pollFds.push_back(serverPollFd);
	std::cout << "Socket created and listening on port " << _port << std::endl;
}


/*
struct sockaddr_in {
	sa_family_t     sin_family;
	in_port_t       sin_port;
	struct  in_addr sin_addr;
	char            sin_zero[8];
};
*/

/*
struct pollfd {
	int fd;
	short events;
	short revents;
};
*/


/*
1. socket()      → Create socket
2. setsockopt()  → Set SO_REUSEADDR (allow port reuse)
3. fcntl()       → Set O_NONBLOCK (non-blocking mode)
4. bind()        → Bind to address (IP + port)
5. listen()      → Start listening for connections
6. accept()      → Accept incoming connections (next step)
*/
