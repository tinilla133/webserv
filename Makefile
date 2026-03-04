# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/07/27 15:55:23 by fvizcaya          #+#    #+#              #
#    Updated: 2025/12/05 18:10:02 by fmorenil         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC = src/main.cpp
SRC += src/Webserver.cpp 
SRC += src/utils.cpp 
SRC += src/Config.cpp 
SRC += src/Request.cpp 
SRC += src/Response.cpp
SRC += src/Client.cpp
SRC += src/CGIHandler.cpp
SRC += src/FileParsing.cpp
SRC += src/Server.cpp
SRC += src/UploadHandler.cpp
SRC += src/ServerBlockConfig.cpp
SRC += src/LocationBlockConfig.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -D PRINT_DEBUG=0 -o $(NAME) $(OBJ)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
