#include "../includes/Server.hpp"

Server::Server() : _port(0), _serverSocketFd(-1), _password("") {
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
	_signal = true;
}

void Server::ServerInit() {
	std::cout << "ServerInit called" << std::endl;
	CreateSocket();

	while (this->_signal == false) {
		int ret = poll(this->_pollFds.data(), this->_pollFds.size(), -1); //! Explain poll what it does in the program
		if (ret == -1 && this->_signal == false) {
			throw std::runtime_error("poll() failed");
		}
		for (int i = 0; i < this->_pollFds.size(); i++) {
			if (this->_pollFds[i].revents & POLLIN) {
				if (this->_pollFds[i].fd == this->_serverSocketFd) {
					acceptClient();
				} else {
					handleMessage(this->_pollFds[i].fd);
				}
			}
		}
	}
	CloseSockets(); //! Need to code this
}

void Server::ClearClients(int fd) {
	for (int i = 0; i < this->_clients.size(); i++) {
		if (this->_clients[i].getSocketFd() == fd) {
			this->_clients.erase(this->_clients.begin() + i);
			break;
		}
	}
	for (int i = 0; i < this->_pollFds.size(); i++) {
		if (this->_pollFds[i].fd == fd) {
			this->_pollFds.erase(this->_pollFds.begin() + i);
			break;
		}
	}
	std::cout << "Client [" << fd << "] cleared" << std::endl;
}

void Server::handleMessage(int fd) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (bytesReceived == -1) {
		std::cout << "Client disconnected: " << fd << std::endl;
		ClearClients(fd);
		close(fd);
	} else {
		buffer[bytesReceived] = '\0';
		std::cout << "Client [" << fd << "] sent message: " << buffer << std::endl;
	}
}

void Server::acceptClient() {
	std::cout << "acceptClient called" << std::endl;

	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(_serverSocketFd, (struct sockaddr *)&clientAddr, clientAddrLen);
	if (clientFd == -1) {
		throw std::runtime_error("Failed to accept client: accept() failed");
	}

	Client cli(clientFd);
	cli.setIp(inet_ntoa(clientAddr.sin_addr));
	if (cli.getIp() == "") {
		throw std::runtime_error("Failed to get client IP: inet_ntoa() failed");
	}

	if (fcntl(cli.getSocketFd(), F_SETFL, O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set socket option to non-blocking: fcntl() failed");
	}

	struct pollfd clientPollFd;
	clientPollFd.fd = cli.getSocketFd();
	clientPollFd.events = POLLIN;
	clientPollFd.revents = 0;

	this->_pollFds.push_back(clientPollFd);
	this->_clients.push_back(cli);
	std::cout << "Client accepted: " << cli.getSocketFd() << std::endl;
}

void Server::CloseSockets() {
	std::cout << "CloseSockets called" << std::endl;
	for (int i = 0; i < this->_pollFds.size(); i++) {
		close(this->_pollFds[i].fd);
	}
	this->_pollFds.clear();
}

void Server::CreateSocket() {
	std::cout << "CreateSocket called" << std::endl;
	int en = 1;
	struct sockaddr_in add;
	struct pollfd serverPollFd;

	add.sin_family = AF_INET;
	add.sin_addr.s_addr = INADDR_ANY;
	add.sin_port = htons(this->_port);


	_serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocketFd == -1) {
		throw std::runtime_error("Failed to create socket: socket() failed");
	}

	if (setsockopt(_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1) {
		throw std::runtime_error("Failed to set socket options: setsockopt() failed");
	}

	if (fcntl(_serverSocketFd, F_SETFL, O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set socket option to non-blocking: fcntl() failed");
	}

	if (bind(_serverSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1) {
		throw std::runtime_error("Failed to bind socket: bind() failed");
	}

	if (listen(_serverSocketFd, SOMAXCONN) == -1) {
		throw std::runtime_error("Failed to listen on socket: listen() failed");
	}

	//! Re explain this
	serverPollFd.fd = _serverSocketFd;
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
