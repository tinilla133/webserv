/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/11 00:11:28 by fmorenil          #+#    #+#             */
/*   Updated: 2025/12/06 16:45:25 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Client.hpp>
#include <Webserver.hpp>

Client::Client(int client_fd) : fd(client_fd), request_completed(false), response_ready(false), bytes_sent(0) {}

Client::~Client() {
    if (fd != -1) {
        close(fd);
    }
}

bool Client::receiveData() {
    char buffer[4096];
    ssize_t bytes_received = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);

    if (bytes_received > 0) {
        request_buffer.append(buffer, bytes_received);

        // Look for the end of the headers
        size_t header_end = request_buffer.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            // If we already found the end of the headers, check for Content-Length
            std::string headers = request_buffer.substr(0, header_end);
            size_t cl_pos = headers.find("Content-Length:");
            size_t body_start = header_end + 4;

            if (cl_pos != std::string::npos) {
                // Extract the Content-Length value
                cl_pos += 15;
                while (cl_pos < headers.size() && isspace(headers[cl_pos])) cl_pos++;
                size_t end_pos = headers.find("\r\n", cl_pos);
                std::string cl_value = headers.substr(cl_pos, end_pos - cl_pos);
                int content_length = atoi(cl_value.c_str());

                // If we have already received the entire body, mark request complete
                if (request_buffer.size() >= body_start + content_length)
                    request_completed = true;
            } else {
                // If there is no Content-Length (for example GET), it is already complete
                request_completed = true;
            }
        }
        return true;
    } 
    else if (bytes_received == 0) {
        // Client closed the connection
        return false;
    } 
    else {
        // recv() == -1: no data right now, we'll retry when poll() notifies us/////////
        return true;
    }
}

bool Client::isRequestComplete() {
    return request_completed;
}

std::string Client::getRequest() {
    return request_buffer;
}

bool Client::sendData() {
    if (response_buffer.empty() || bytes_sent >= response_buffer.size()) {
        return (true);
    }

    const char* data = response_buffer.c_str() + bytes_sent;
    size_t to_send = response_buffer.size() - bytes_sent;

    ssize_t bytes_sent_now = send(fd, data, to_send, MSG_DONTWAIT);
    
    if (bytes_sent_now > 0) {
        bytes_sent += bytes_sent_now;
        return (true);
    } else if (bytes_sent_now == 0) {
        // Should not happen with send(), but handle gracefully
        return (true);
    } else {
        // bytes_sent_now == -1
        // For non-blocking sockets, socket not ready to send is not an error
        // poll() will tell us when socket is ready for writing
        return (true);
    }
}

bool Client::isResponseReady() {
    return response_ready;
}

void Client::setResponse(const std::string &response) {
    response_buffer = response;
    response_ready = true;
    bytes_sent = 0;
}

int Client::getFd() const {
    return fd;
}

bool Client::isReady() const {
    return request_completed && !response_ready;
}

bool Client::isResponseComplete() const {
    return response_ready && (bytes_sent >= response_buffer.size());
}
