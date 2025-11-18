#include <iostream>
#include "Client.hpp";
#include <vector>

class Channel {
	public:
		Channel(std::string name);
		~Channel();

		void addClient(Client *client);
		void removeClient(Client *client);
		void addOperator(Client *client);
		void removeOperator(Client *client);
		void setTopic(std::string topic);
		void setPassword(std::string password);
		void setMaxUsers(int maxUsers);

		std::string getTopic();
		std::string getPassword();
		int getMaxUsers();
		std::string getName();
		std::vector <Client *> getClients();
		std::vector <Client *> getOperators();

		void broadcast(std::string message);

	private:
		int _maxUsers;
		std::string _name;
		std::vector <Client *> _clients;
		std::vector <Client *> _operators;
		std::string _password;
		std::string _topic;
};
