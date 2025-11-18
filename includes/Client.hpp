#include <iostream>

class Client {
	public:
		Client(int fd);
		~Client();

		// Getters
		int getSocketFd();
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
		int _fd;
		std::string _ip;

		std::string _nickname;
		std::string _username;
		bool _authenticated;
		std::vector<std::string> _channels;
};
