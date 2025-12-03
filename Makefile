NAME = ircserv

CC = c++
CCFLAGS = -Wall -Wextra -Werror -std=c++98

OBJS_DIR = objs/
SRCS_DIR = src/

FILES = main.cpp \
		Server.cpp \
		Client.cpp \
		Channel.cpp \
		Exceptions.cpp \
		commandsirc.cpp

SRCS = $(addprefix $(SRCS_DIR), $(FILES))
OBJS = $(addprefix $(OBJS_DIR), $(FILES:.cpp=.o))

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp
	@mkdir -p $(OBJS_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CCFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@$(CC) $(CCFLAGS) -o $(NAME) $(OBJS)

all: $(NAME)

clean:
	@echo "Cleaning: $(OBJS_DIR)"
	@rm -f $(OBJS)
	@rm -rf $(OBJS_DIR)

fclean: clean
	@echo "Cleaning: $(NAME)"
	@rm -f $(NAME)

re: fclean all

x: all
	./$(NAME) 8888 pa

.PHONY: all clean fclean re
