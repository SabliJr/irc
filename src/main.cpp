#include "../includes/Server.hpp"

bool check_args(int ac, char **av)
{
	// Check number of arguments
	// Check if av[0] is ./ircserv
	// Check if av[1] is a valid port
	// Check if av[2] is a password

	if (ac != 3) {
		std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
		return false;
	}

	if (strcmp(av[0], "./ircserv") != 0) {
		std::cout << "av[0] is not ./ircserv" << std::endl;
		return false;
	}

	if (atoi(av[1]) < 1024 || atoi(av[1]) > 65535) {
		std::cout << "av[1] is not a valid port" << std::endl;
		return false;
	}
	if (strlen(av[2]) < 1) {
		std::cout << "Password is empty" << std::endl;
		return false;
	}
	return true;
}

int main(int ac, char **av)
{
	if (!check_args(ac, av)) {
		std::cout << "Arguments are invalid" << std::endl;
		return 1;
	}

	try {
		signal(SIGINT, Server::SignalHandler);
		signal(SIGTERM, Server::SignalHandler);
		Server ser(atoi(av[1]), av[2]);

	} catch (const std::exception &e) {
		std::cout << RED "Error: " << e.what() << RESET "\n" << std::endl;
		return 1;
	}

	return 0;
}

