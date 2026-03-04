/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 15:57:19 by fmorenil          #+#    #+#             */
/*   Updated: 2025/12/06 14:22:19 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <CGIHandler.hpp>
#include <Request.hpp>
#include <Webserver.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <cctype>
#include <cstring>

CGIHandler::CGIHandler(const Request& _request, const std::string& _scriptPath, const std::string& _cgiExecutor)
    : request(_request), scriptPath(_scriptPath), cgiExecutor(_cgiExecutor) {
    setupEnvironment();
}

CGIHandler::~CGIHandler() {}

void CGIHandler::setupEnvironment() {
    // Standard CGI environment variables
    _env["REQUEST_METHOD"] = request.getMethod();
    _env["SCRIPT_NAME"] = scriptPath;
    _env["SCRIPT_FILENAME"] = scriptPath;
    _env["PATH_INFO"] = request.path;
    _env["QUERY_STRING"] = "";
    
    // Extract query string if it exists
    size_t queryPos = request.path.find('?');
    if (queryPos != std::string::npos) {
        _env["QUERY_STRING"] = request.path.substr(queryPos + 1);
        _env["PATH_INFO"] = request.path.substr(0, queryPos);
    }
    
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_SOFTWARE"] = "webserv/1.0";
    _env["REQUEST_URI"] = request.path;
    _env["DOCUMENT_ROOT"] = "/home/fmorenil/webserv/www";
    
    // Request headers
    if (request.headers.find("Content-Type") != request.headers.end()) {
        _env["CONTENT_TYPE"] = request.headers.at("Content-Type");
    }
    
    if (request.headers.find("Content-Length") != request.headers.end()) {
        _env["CONTENT_LENGTH"] = request.headers.at("Content-Length");
    } else if (!request.body.empty()) {
        std::ostringstream oss;
        oss << request.body.length();
        _env["CONTENT_LENGTH"] = oss.str();
    }
    
    // Additional HTTP header variables
    for (std::map<std::string, std::string>::const_iterator it = request.headers.begin(); 
         it != request.headers.end(); ++it) {
        std::string headerName = "HTTP_" + it->first;
        // Convert to uppercase and replace - with _
        for (size_t i = 0; i < headerName.length(); i++) {
            if (headerName[i] == '-')
                headerName[i] = '_';
            else
                headerName[i] = std::toupper(headerName[i]);
        }
        _env[headerName] = it->second;
    }
    _env["REDIRECT_STATUS"] = "200";

}

void CGIHandler::setupPipes(int pipeFd[2]) {
    int flags;

    // Make the read pipe non-blocking
    flags = fcntl(pipeFd[0], F_GETFL);
    fcntl(pipeFd[0], F_SETFL, flags | O_NONBLOCK);
}

pid_t CGIHandler::forkAndExec(int pipeFd[2]) {
    pid_t pid = fork();
    
    if (pid == -1) {
        ERR_PRINT("Fork failed: " + std::string(strerror(errno)));
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        close(pipeFd[0]); // Close read end
        
        // Redirect stdout to the pipe
        dup2(pipeFd[1], STDOUT_FILENO);
        close(pipeFd[1]);
        
        // If it's POST, prepare stdin with the body
            // If it's POST, connect the body to the CGI stdin
    if (request.getMethod() == "POST") {
        int stdinPipe[2];
        if (pipe(stdinPipe) == -1) {
            ERR_PRINT("Failed to create stdin pipe: " + std::string(strerror(errno)));
            exit(1);
        }

        pid_t writer_pid = fork();
        if (writer_pid == -1) {
            ERR_PRINT("Fork for POST writer failed: " + std::string(strerror(errno)));
            exit(1);
        }

        if (writer_pid == 0) {
            // Child writer process: send the body
            close(stdinPipe[0]);
            if (!request.body.empty()) {
                write(stdinPipe[1], request.body.c_str(), request.body.length());
            }
            close(stdinPipe[1]);
            exit(0);
        }

        // CGI process: read the body from stdin
        close(stdinPipe[1]);
        dup2(stdinPipe[0], STDIN_FILENO);
        close(stdinPipe[0]);
    }

        
        // Build argv and envp
        std::vector<char*> argv = buildArgv();
        std::vector<char*> envp = buildEnvp();
        

        // Execute the script
        execve(argv[0], &argv[0], &envp[0]);
        
        // If we reach this point, execve failed
        // Write an error message and close descriptors
        const char* error_msg = "CGI execution failed\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        
        // Close all descriptors and exit using abort (or an infinite loop as a last resort)
        while(1) {
            // Infinite loop to prevent the child process from continuing
            // The parent process will kill it with the timeout
        }
    }
    
    return (pid);
}

std::vector<char*> CGIHandler::buildEnvp() const {
    static char envBuffers[50][512];
    static int envCount = 0;
    std::vector<char*> envp;
    
    envCount = 0;  // Reset counter
    
    for (std::map<std::string, std::string>::const_iterator it = _env.begin();
         it != _env.end() && envCount < 50; ++it) {
        std::string envVar = it->first + "=" + it->second;
        strncpy(envBuffers[envCount], envVar.c_str(), sizeof(envBuffers[envCount]) - 1);
        envBuffers[envCount][sizeof(envBuffers[envCount]) - 1] = '\0';
        envp.push_back(envBuffers[envCount]);
        envCount++;
    }
    
    envp.push_back(NULL);
    return envp;
}

std::vector<char*> CGIHandler::buildArgv() const {
    // We use large static buffers to avoid memory issues
    static char interpreterBuffer[256];
    static char scriptBuffer[256];
    std::vector<char*> argv;
    
    std::string interpreter = cgiExecutor;
    if (interpreter.empty()) {
        interpreter = getInterpreter(scriptPath);
    }
    if (!interpreter.empty()) {
        strncpy(interpreterBuffer, interpreter.c_str(), sizeof(interpreterBuffer) - 1);
        interpreterBuffer[sizeof(interpreterBuffer) - 1] = '\0';
        argv.push_back(interpreterBuffer);
    }
    
    strncpy(scriptBuffer, scriptPath.c_str(), sizeof(scriptBuffer) - 1);
    scriptBuffer[sizeof(scriptBuffer) - 1] = '\0';
    argv.push_back(scriptBuffer);
    
    argv.push_back(NULL);
    return argv;
}

std::string CGIHandler::execute() {
    int pipeFd[2];
    
    if (pipe(pipeFd) == -1) {
        ERR_PRINT("Pipe creation failed: " + std::string(strerror(errno)));
        return "";
    }

    setupPipes(pipeFd);
    pid_t childPid = forkAndExec(pipeFd);
    
    if (childPid == -1) {
        close(pipeFd[0]);
        close(pipeFd[1]);
        return "";
    }
    
    // Parent process
    close(pipeFd[1]); // Close write end
    
    std::string result;
    char buffer[4096];
    ssize_t bytesRead;
    
    // Use poll for the CGI timeout
    struct pollfd pfd;
    pfd.fd = pipeFd[0];
    pfd.events = POLLIN;
    
    int timeout = 30000; // 30 second timeout
    int pollResult = poll(&pfd, 1, timeout);
    
    if (pollResult > 0 && (pfd.revents & POLLIN)) {
        // Read CGI output
        while ((bytesRead = read(pipeFd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
            
            // Check if more data is available
            pollResult = poll(&pfd, 1, 0);
            if (pollResult <= 0) break;
        }
    } else if (pollResult == 0) {
        // Timeout - kill the child process
        kill(childPid, SIGKILL);
        ERR_PRINT("CGI script timeout");
    }
    
    close(pipeFd[0]);
    
    // Wait for the child process to finish
    int status;
    waitpid(childPid, &status, 0);
    
    return (result);
}

bool CGIHandler::isCGIScript(const std::string& path) {
    // Verificar extensiones CGI comunes
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    
    std::string extension = path.substr(dotPos);
    return (extension == ".php" || extension == ".py" || extension == ".pl" || 
            extension == ".sh" || extension == ".cgi");
}

std::string CGIHandler::getInterpreter(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    
    std::string extension = path.substr(dotPos);
    
    if (extension == ".php") {
        return "/usr/bin/php-cgi";
    } else if (extension == ".py") {
        return "/usr/bin/python3";
    } else if (extension == ".pl") {
        return "/usr/bin/perl";
    } else if (extension == ".sh" || extension == ".cgi") {
        return "/bin/bash";
    }
    
    return "";
}
