#include "../includes/Client.hpp"

Client::Client(int fd) : _fd(fd), _ip(""), _nickname(""), _username(""), _authenticated(false), _channels(std::vector<std::string>()), _readbuffer("") {}

Client::~Client() {}

int Client::getSocketFd() {
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

std::string Client::getReadbuffer() {
	return this->_readbuffer;
}

void Client::setReadbuffer(std::string readbuffer) {
	this->_readbuffer = readbuffer;
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

void Client::setIp(std::string ip) {
	this->_ip = ip;
}

void Client::addChannel(std::string channel) {
	this->_channels.push_back(channel);
}

std::vector<std::string> Client::getPendingCommands() {
	return this->_pending_commands;
}

void Client::addPendingCommand(std::string command) {
	this->_pending_commands.push_back(command);
}
