# c++ -Wall -Wextra -Werror -std=c++98 main.cpp Server.cpp -o ircserv

NAME = ircserv

SRCS = srcs/main.cpp \
	   srcs/Server.cpp \
	   srcs/Client.cpp \

OBJSDIR = objs
OBJS = $(SRCS:%.cpp=$(OBJSDIR)/%.o)
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJSDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "$(OBJSDIR)/$< compiled"

clean:
	rm -rf $(OBJSDIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: clean fclean re