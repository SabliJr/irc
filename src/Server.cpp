#include "../includes/Server.hpp"

bool Server::_signal = false;
std::vector<Client> Server::_clients;

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
	std::cout << BLUE << "Signal received: " << signum << RESET << std::endl;
	_signal = true;
}

void Server::ServerInit() {
	std::cout << GREEN << "ServerInit called" << RESET << std::endl;
	CreateSocket();

	while (this->_signal == false) {
		int ret = poll(this->_pollFds.data(), this->_pollFds.size(), -1); //! Explain poll what it does in the program
		if (ret == -1 && this->_signal == false) {
			throw std::runtime_error("poll() failed");
		}
		for (size_t i = 0; i < this->_pollFds.size(); i++) {
			if (this->_pollFds[i].revents & POLLIN) {
				if (this->_pollFds[i].fd == this->_serverSocketFd) {
					acceptClient();
				} else {
					Client *client = getClientByFd(this->_pollFds[i].fd);
					if (client != NULL) {
						handleMessage(this->_pollFds[i].fd, client);
					} else {
						std::cout << "Unknown client fd: " << this->_pollFds[i].fd << std::endl;
					}
				}
			}
		}
	}
	CloseSockets(); //! Need to code this
}

void Server::ClearClients(int fd) {
	std::cout << GREEN << "ClearClients called" << RESET << std::endl;
	for (size_t i = 0; i < this->_clients.size(); i++) {
		if (this->_clients[i].getSocketFd() == fd) {
			this->_clients.erase(this->_clients.begin() + i);
			break;
		}
	}
	for (size_t i = 0; i < this->_pollFds.size(); i++) {
		if (this->_pollFds[i].fd == fd) {
			this->_pollFds.erase(this->_pollFds.begin() + i);
			break;
		}
	}
	std::cout << "Client [" << fd << "] cleared" << std::endl;
}

void Server::parseCommand(Client *client, const std::string &line) {
	std::cout << GREEN << "parseCommand called" << RESET << std::endl;

	std::cout << "[parseCommand] fd=" << client->getSocketFd() << " line=" << line << std::endl;
}


void Server::handleMessage(int fd, Client *client) {
	std::cout << GREEN << "handleMessage called" << RESET << std::endl;

	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (bytesReceived <= 0) {
		if (bytesReceived == 0) {
			std::cout << "Client " << fd << " disconnected" << std::endl;
		} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cout << "recv() error on client " << fd << ": " << strerror(errno) << std::endl;
		}
		ClearClients(fd);
		close(fd);
		return;
	}

	if (bytesReceived > 512) {
		std::cout << "Message is too long" << std::endl;
		return;
	}

	buffer[bytesReceived] = '\0';

	client->setReadbuffer(client->getReadbuffer() + buffer);

	while(1) {
		size_t pos = client->getReadbuffer().find("\r\n");
		if (pos == std::string::npos) {
			break;
		}

		std::string command = client->getReadbuffer().substr(0, pos);
		client->setReadbuffer(client->getReadbuffer().substr(pos + 2));
		client->addPendingCommand(command);
	}
	const std::vector<std::string> &pendingCommands = client->getPendingCommands();
	for (size_t i = 0; i < pendingCommands.size(); i++) {
			parseCommand(client, pendingCommands[i]);
	}
		//! Do this function later clearPendingCommands
		// client->clearPendingCommands();
}

Client *Server::getClientByFd(int fd) {
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i].getSocketFd() == fd) {
			return &_clients[i];
		}
	}
	return NULL;
}

void Server::acceptClient() {
	std::cout << GREEN << "acceptClient called" << RESET << std::endl;

	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(_serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
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
	std::cout << GREEN << "CloseSockets called" << RESET << std::endl;
	for (size_t i = 0; i < this->_pollFds.size(); i++) {
		close(this->_pollFds[i].fd);
	}
	this->_pollFds.clear();
}

void Server::CreateSocket() {
	std::cout << GREEN << "CreateSocket called" << RESET << std::endl;
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
