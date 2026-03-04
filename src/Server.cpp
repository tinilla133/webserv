/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 20:07:44 by aurodrig          #+#    #+#             */
/*   Updated: 2025/12/06 13:57:02 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Webserver.hpp>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <Server.hpp>
#include <fcntl.h>

Server::Server(int port) : port(port)
{
    DBG_PRINT("Creating Server object on port " << port);
    // Create socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        ERR_PRINT("Failed to create socket (socket() failed)");
        return;
    }

    //Reuse address
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        ERR_PRINT("Failed to set SO_REUSEADDR option");
        close(listen_fd);
        listen_fd = -1;
        return;
    }

    //Configure address
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //Bind
    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        ERR_PRINT("bind() failed on port " << port);
        close(listen_fd);
        listen_fd = -1;
        return;
    }

    //Listen
    if (listen(listen_fd, 10) < 0)
    {
        ERR_PRINT("listen() failed on port " << port);
        close(listen_fd);
        listen_fd = -1;
        return;
    }

    if (fcntl(listen_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        ERR_PRINT("Failed to set non-blocking mode on listen_fd");
        close(listen_fd);
        listen_fd = -1;
        return;
    }
}

Server::~Server()
{
    if (listen_fd != -1)
        close(listen_fd);
    INF_PRINT("Server socket closed successfully");
}

int Server::getFd() const { return listen_fd; }
int Server::getPort() const { return port; }

int Server::acceptClient() const
{
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0)
    {
        ERR_PRINT("accept() failed while trying to accept a new client");
        return -1;
    }

    DBG_PRINT("New client accepted: fd " << client_fd);
    return client_fd;
}
