#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>

class Client {
	public:
		Client(int fd);
		~Client();

		// Getters
		int getFd();
		std::string getIp();
		std::string getNickname();
		std::string getUsername();
		bool getAuthenticated();
		std::vector<std::string> getChannels();

		// Setters
		void setNickname(std::string nickname);
		void setUsername(std::string username);
		void setAuthenticated(bool authenticated);
		void addChannel(std::string channel);
		void removeChannel(std::string channel);

	private:
		int							_fd;
		std::string					_hostname;
		std::string					_username;
		std::string					_nickname;
		std::string					_ip;
		bool						_authenticated;
		bool						_operator;
		std::vector<std::string>	_channels;
};

#endif
