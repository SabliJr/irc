#include "../includes/Client.hpp"

//&			CONSTRUCTORS AND DESTRUCTOR

Client::Client(int fd) : _fd(fd), _ip(""), _nickname(""), _username(""), _authenticated(false), _channels(std::vector<std::string>()) {}

Client::~Client() {}

//$ 			SET AND GET FUNCTIONS

int Client::getFd() {
	return this->_fd;
}

std::string Client::getIp() {
	return this->_ip;
}

std::string Client::getNickname() {
	return this->_nickname;
}

std::string Client::getUsername() {
	return this->_username;
}

bool Client::getAuthenticated() {
	return this->_authenticated;
}

std::vector<std::string> Client::getChannels() {
	return this->_channels;
}

void Client::setNickname(std::string nickname) {
	this->_nickname = nickname;
}

void Client::setUsername(std::string username) {
	this->_username = username;
}

void Client::setAuthenticated(bool authenticated) {
	this->_authenticated = authenticated;
}

//€ 				MEMBER FUNCTIONS				

void Client::addChannel(std::string channel) {
	this->_channels.push_back(channel);
}
