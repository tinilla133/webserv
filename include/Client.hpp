/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/11 00:07:31 by fmorenil          #+#    #+#             */
/*   Updated: 2025/10/14 19:43:13 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

class Client
{
    private:
        int fd;
        std::string request_buffer;
        std::string response_buffer;
        bool request_completed;
        bool response_ready;
        size_t bytes_sent;

    public:
        Client(int client_fd);
        ~Client();

        // Request handling
        bool receiveData();
        bool isRequestComplete();
        std::string getRequest();

        // Response handling
        bool sendData();
        bool isResponseReady();
        void setResponse(const std::string &response);

        // State
        int getFd() const;
        bool isReady() const;
        bool isResponseComplete() const;
};
