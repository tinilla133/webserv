// /* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserver.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 15:46:08 by fmorenil          #+#    #+#             */
/*   Updated: 2025/09/23 20:35:54 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Webserver.hpp>
#include <utils.hpp>
#include <Request.hpp>
#include <Response.hpp>
#include <CGIHandler.hpp>
#include <Config.hpp>
#include <UploadHandler.hpp>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <vector>
#include <fstream>
#include <set>
#include <sys/stat.h>
#include <dirent.h>

static inline void strip_all_trailing_slashes(std::string& s) {
    while (s.size() > 1 && s[s.size()-1] == '/') {
        s.erase(s.size()-1);
    }
}

static inline void strip_suffix_if_present(std::string& s, const std::string& suffix) {
    if (suffix.empty()) return;
    if (s.size() < suffix.size()) return;
    const std::string::size_type start = s.size() - suffix.size();
    if (s.compare(start, suffix.size(), suffix) == 0) {
        s.erase(start);
    }
}

static bool path_ends_with_exact(std::string path, std::string suffixPath, const std::string& documentIndex) {
    strip_all_trailing_slashes(path);
    strip_all_trailing_slashes(suffixPath);

    strip_suffix_if_present(path, documentIndex);
    strip_all_trailing_slashes(path);

    if (suffixPath.empty()) {
        return false;
    }
    if (path.size() < suffixPath.size()) {
        return false;
    }
    const std::string::size_type start = path.size() - suffixPath.size();
    if (path.compare(start, suffixPath.size(), suffixPath)) {
        return false;
    }
    if (!start) {
        return true;
    }
    return path[start - 1] == '/';
}

bool WebServer::loadConfig(const std::string &path)
{
    configFileParser parser;
    parser.setFilePath(path);
    parser.setConfigObject(serverConfig);
    return (parser.parseFile());
}

const Config& WebServer::getConfig() const
{
    return (serverConfig);
}

void WebServer::setConfig(const Config &newConfig)
{
    serverConfig = newConfig;
}

Config &WebServer::getServerConfig(void)
{
    return serverConfig;
}

WebServer::WebServer() {}

WebServer::~WebServer() {
    for (std::map<int, Client*>::iterator it = clients.begin(); 
         it != clients.end(); ++it) {
        delete it->second;
    }
    clients.clear();
    
    for (size_t i = 0; i < servers.size(); ++i) {
        delete servers[i];
    }
    servers.clear();
}

bool WebServer::initMultipleServers() {
    if (serverConfig.getNumServerBlocks() == 0) {
        ERR_PRINT("No server blocks configured.");
        return false;
    }
    
    std::set<int> uniquePorts;
    
    // Identify unique ports and create servers
    for (int i = 0; i < serverConfig.getNumServerBlocks(); i++) {
        const ServerBlock& block = serverConfig.getServerBlockIndex(i);
        int port = block.getListeningPort();
        
        // Only create one server per unique port
        if (uniquePorts.find(port) == uniquePorts.end()) {
            uniquePorts.insert(port);
            
            Server* newServer = new Server(port);
            if (newServer->getFd() == -1) {
                ERR_PRINT("Server failed to initialize on port " << port);
                delete newServer;
                return false;
            }
            
            servers.push_back(newServer);
            fdToServerIndex[newServer->getFd()] = servers.size() - 1;
            
            //Add to the poll vector
            pollfd server_poll;
            server_poll.fd = newServer->getFd();
            server_poll.events = POLLIN;
            fds.push_back(server_poll);
            
            INF_PRINT("Server initialized on port " << port << " (fd=" << newServer->getFd() << ")");
        }
    }
    
    INF_PRINT("Total servers initialized: " << servers.size());
    return true;
}

bool WebServer::isListenerSocket(int fd) const {
    return fdToServerIndex.find(fd) != fdToServerIndex.end();
}

const ServerBlock& WebServer::selectServerBlock(int listener_fd, const Request& req) const {
    //Find all ServerBlocks that listen on this port
    Server* currentServer = NULL;
    std::map<int, int>::const_iterator serverIt = fdToServerIndex.find(listener_fd);
    if (serverIt != fdToServerIndex.end()) {
        currentServer = servers[serverIt->second];
    }
    
    if (!currentServer) {
        //Fallback to the first server block if there are issues
        return serverConfig.getServerBlockIndex(0);
    }
    
    int currentPort = currentServer->getPort();
    
    //Search for ServerBlocks that match this port
    std::vector<int> candidateServers;
    for (int i = 0; i < serverConfig.getNumServerBlocks(); i++) {
        const ServerBlock& block = serverConfig.getServerBlockIndex(i);
        if (block.getListeningPort() == currentPort) {
            candidateServers.push_back(i);
        }
    }
    
    if (candidateServers.empty()) {
        return serverConfig.getServerBlockIndex(0);
    }
    
    //If there is a Host header, look for a matching server_name
    std::map<std::string, std::string>::const_iterator hostIt = req.headers.find("host");
    if (hostIt != req.headers.end()) {
        std::string hostHeader = hostIt->second;
        
        // Remove port from the Host header if present (e.g., "localhost:8080" -> "localhost")
        size_t colonPos = hostHeader.find(':');
        if (colonPos != std::string::npos) {
            hostHeader = hostHeader.substr(0, colonPos);
        }
        
        //Look for exact match with server_name
        for (size_t i = 0; i < candidateServers.size(); i++) {
            const ServerBlock& block = serverConfig.getServerBlockIndex(candidateServers[i]);
            const std::vector<std::string>& serverNames = block.getServerName();
            
            for (size_t j = 0; j < serverNames.size(); j++) {
                if (serverNames[j] == hostHeader) {
                    DBG_PRINT("Selected server by host match: " << hostHeader << " (port " << currentPort << ")");
                    return block;
                }
            }
        }
    }
    
    //Fallback: use the first ServerBlock for this port
    const ServerBlock& fallbackBlock = serverConfig.getServerBlockIndex(candidateServers[0]);
    DBG_PRINT("Selected fallback server for port " << currentPort);
    return fallbackBlock;
}

EffectiveConfig WebServer::mergeConfigurations(const ServerBlock& server, const LocationBlock* location) const {
    EffectiveConfig config;
    
    //Start with server configuration as base
    config.documentRoot = server.getDocumentRoot();
    config.serverRoot = server.getDocumentRoot();
    config.errorPageRoot = server.getDocumentRoot();
    config.indexPath = server.getIndexPath();
    config.allowMethods = server.getAllowMethods();
    config.clientMaxBodySize = server.getClientMaxBodySize();
    config.autoIndex = server.getAutoIndex();
    config.uploadEnable = server.getUploadEnable();
    config.uploadStore = server.getUploadStore();
    config.cgiPass = server.getCgiPass();
    config.errorPageMap = server.getErrorPageMap();
    
    //Override with location configuration if present
    if (location) {
        DBG_PRINT("Applying location block overrides for path: " << location->getLocationPath());
        
        //Override only non-empty/non-default values from location
        if (!location->getDocumentRoot().empty()) {
            config.documentRoot = location->getDocumentRoot();
        }
        if (!location->getIndexPath().empty()) {
            config.indexPath = location->getIndexPath();
        }
        if (!location->getAllowMethods().empty()) {
            config.allowMethods = location->getAllowMethods();
        }
        if (!location->getClientMaxBodySize().empty()) {
            config.clientMaxBodySize = location->getClientMaxBodySize();
        }
        //Location can override boolean values
        if (location->getAutoIndex() != server.getAutoIndex()) {
            config.autoIndex = location->getAutoIndex();
        }
        if (location->getUploadEnable() != server.getUploadEnable()) {
            config.uploadEnable = location->getUploadEnable();
        }
        if (!location->getUploadStore().empty()) {
            config.uploadStore = location->getUploadStore();
        }
        if (!location->getCgiPass().empty()) {
            config.cgiPass = location->getCgiPass();
        }
        //Merge error pages (location overrides server)
        const std::map<int, std::string>& locationErrors = location->getErrorPageMap();
        DBG_PRINT("Location has " << locationErrors.size() << " error pages");
        for (std::map<int, std::string>::const_iterator it = locationErrors.begin(); 
             it != locationErrors.end(); ++it) {
            DBG_PRINT("Adding location error page: " << it->first << " -> " << it->second);
            config.errorPageMap[it->first] = it->second;
        }
        if (!locationErrors.empty()) {
            if (!location->getDocumentRoot().empty())
                config.errorPageRoot = location->getDocumentRoot();
            else
                config.errorPageRoot = server.getDocumentRoot();
        }
    }
    
    return config;
}

void WebServer::applyCustomErrorPage(Response& res, const EffectiveConfig& config) const {
    if (res.status_code < 400)
        return;

    DBG_PRINT("Looking for custom error page for status " << res.status_code);
    DBG_PRINT("Available error pages: " << config.errorPageMap.size());
    
    std::map<int, std::string>::const_iterator it = config.errorPageMap.find(res.status_code);
    if (it == config.errorPageMap.end()) {
        DBG_PRINT("No custom error page found for status " << res.status_code);
        return;
    }

    std::string pagePath = it->second;
    if (pagePath.empty()) {
        DBG_PRINT("Error page path is empty");
        return;
    }

    DBG_PRINT("Found error page config: " << pagePath);
    std::string fullPath;
    const std::string& baseRoot = config.errorPageRoot.empty() ? config.documentRoot : config.errorPageRoot;
    DBG_PRINT("Base root: " << baseRoot << ", Server root: " << config.serverRoot);
    
    if (pagePath[0] == '/') {
        fullPath = baseRoot + pagePath;
    } else {
        fullPath = baseRoot + "/" + pagePath;
    }

    DBG_PRINT("Trying to load error page from: " << fullPath);
    std::string content = readFileNonBlocking(fullPath);
    if (!content.empty()) {
        DBG_PRINT("Successfully loaded error page (" << content.size() << " bytes)");
        res.body = content;
        res.headers["Content-Type"] = getContentType(fullPath);
    } else {
        DBG_PRINT("Failed to load error page from: " << fullPath);
    }
}

void WebServer::run() {
    INF_PRINT("Server running with " << servers.size() << " listeners");

    while (true) {
        int activity = poll(fds.data(), fds.size(), -1);
        if (activity < 0) {
            ERR_PRINT("poll failed");
            continue;
        } else if (activity == 0) {
            continue;
        }

        // Process events in reverse order to avoid problems when removing elements
        for (int i = fds.size() - 1; i >= 0; i--) {
            // Check for errors or hangup first
            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (!isListenerSocket(fds[i].fd)) {
                    // Client connection has an error
                    DBG_PRINT("Client connection error/hangup: fd=" << fds[i].fd);
                    cleanupClient(fds[i].fd);
                }
                continue;
            }
            
            if (fds[i].revents & POLLIN) {
                if (isListenerSocket(fds[i].fd)) {
                    // New connection on one of our listeners
                    handleNewConnection(fds[i].fd);
                } else {
                    // Data from existing client
                    handleClientData(fds[i].fd);
                }
            } else if (fds[i].revents & POLLOUT) {
                // Client ready to send response
                handleClientSend(fds[i].fd);
            }
        }

        // Process clients that are ready to generate a response
        for (std::map<int, Client*>::iterator it = clients.begin(); 
             it != clients.end(); ++it) {
            if (it->second->isReady()) {
                processClientRequest(it->first);
            }
        }
    }
}

void WebServer::handleNewConnection(int listener_fd)
{
    // Find the server corresponding to this listener
    std::map<int, int>::iterator it = fdToServerIndex.find(listener_fd);
    if (it == fdToServerIndex.end()) {
        ERR_PRINT("Unknown listener fd: " << listener_fd);
        return;
    }
    
    Server* currentServer = servers[it->second];
    int client_fd = currentServer->acceptClient();
    if (client_fd < 0) {
        ERR_PRINT("Failed to accept new client on port " << currentServer->getPort());
        return;
    }

    // Create new client
    Client *new_client = new Client(client_fd);
    clients[client_fd] = new_client;
    
    // Associate client with its listener for future lookups
    clientToListener[client_fd] = listener_fd;

    // Add to polling
    pollfd client_poll = {client_fd, POLLIN, 0};
    fds.push_back(client_poll);

    DBG_PRINT("New client connected: fd " << client_fd << " on port " << currentServer->getPort());
}

void WebServer::handleClientData(int client_fd)
{
    Client *client = clients[client_fd];
    if (!client)
        return;

    if (!client->receiveData())
    {
        DBG_PRINT("Client disconnected or error: fd=" << client_fd);
        cleanupClient(client_fd);
        return;
    }

    if (client->isRequestComplete())
    {
        updatePollEvents(client_fd, POLLOUT);
    }
}

void WebServer::handleClientSend(int client_fd)
{
    Client *client = clients[client_fd];
    if (!client)
        return;

    if (!client->sendData())
    {
        ERR_PRINT("Send error for client: fd=" << client_fd);
        cleanupClient(client_fd);
        return;
    }

    if (client->isResponseComplete())
    {
        DBG_PRINT("Response sent completely: fd=" << client_fd);
        cleanupClient(client_fd);
    }
}

void WebServer::cleanupClient(int client_fd)
{
    std::map<int, Client *>::iterator it = clients.find(client_fd);
    if (it != clients.end())
    {
        delete it->second; 
        clients.erase(it);
    }
    
    // Clear listener tracking
    clientToListener.erase(client_fd);

    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd == client_fd)
        {
            fds.erase(fds.begin() + i);
            break;
        }
    }

    DBG_PRINT("Client cleaned up: fd=" << client_fd);
}

void WebServer::updatePollEvents(int client_fd, short events)
{
    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd == client_fd)
        {
            fds[i].events = events;
            break;
        }
    }
}

void WebServer::processClientRequest(int client_fd) {
    Client* client = clients[client_fd];
    if (!client) return;
    
    if (serverConfig.getNumServerBlocks() == 0) {
        ERR_PRINT("No server blocks configured.");
        return;
    }
    
    std::string request_str = client->getRequest();
    DBG_PRINT("Processing request for fd=" << client_fd);
    
    // Parse request using Request class
    Request req;
    req.parse(request_str);
    
    // Find which listener accepted this client
    std::map<int, int>::iterator listenerIt = clientToListener.find(client_fd);
    if (listenerIt == clientToListener.end()) {
        ERR_PRINT("Could not find listener for client " << client_fd);
        return;
    }
    int listener_fd = listenerIt->second;
    
    // Select the appropriate ServerBlock based on port and Host header
    const ServerBlock& selectedBlock = selectServerBlock(listener_fd, req);
    DBG_PRINT("Selected server block for port " << selectedBlock.getListeningPort());
    
    // Find the most specific location block for this path
    const LocationBlock* matchedLocation = selectedBlock.findBestLocationMatch(req.path);
    if (matchedLocation) {
        DBG_PRINT("Matched location block: " << matchedLocation->getLocationPath());
    } else {
        DBG_PRINT("No location block matched, using server defaults");
    }
    

    EffectiveConfig effectiveConfig = mergeConfigurations(selectedBlock, matchedLocation);
    
    // Handle chunked encoding if present
    if (req.headers.count("transfer-encoding") &&
        req.headers["transfer-encoding"] == "chunked") {
        DBG_PRINT("Decoding chunked body...");
        req.body = decodeChunkedBody(req.body);
    }
    
    size_t maxBody = parseSize(effectiveConfig.clientMaxBodySize);
    if (maxBody > 0) {
        size_t header_end = request_str.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            size_t body_start = header_end + 4;
            size_t body_len = (request_str.size() > body_start) ? request_str.size() - body_start : 0;
            if (body_len > maxBody) {
                Response res;
                res.status_code = 413;
                res.body = "<h1>413 Payload Too Large</h1>";
                applyCustomErrorPage(res, effectiveConfig);
                std::ostringstream len;
                len << res.body.size();
                res.headers["Content-Length"] = len.str();
                if (res.headers.find("Content-Type") == res.headers.end())
                    res.headers["Content-Type"] = "text/html";
                client->setResponse(res.toString());
                updatePollEvents(client_fd, POLLOUT);
                return;
            }
        }
    }

    // Validate request
    if (req.method.empty() || req.path.empty()) {
        Response res;
        res.status_code = 400;
        res.body = "<h1>400 Bad Request</h1>";
        applyCustomErrorPage(res, effectiveConfig);
        if (res.headers.find("Content-Type") == res.headers.end())
            res.headers["Content-Type"] = "text/html";
        std::ostringstream oss;
        oss << res.body.size();
        res.headers["Content-Length"] = oss.str();
        client->setResponse(res.toString());
        updatePollEvents(client_fd, POLLOUT);
        return;
    }
    
    DBG_PRINT("Request: " << req.method << " " << req.path);

    // Construct file path (remove query string if present)
    std::string clean_path = req.path;
    size_t query_pos = clean_path.find('?');
    if (query_pos != std::string::npos) {
        clean_path = clean_path.substr(0, query_pos);
    }
    
    std::string resource_path = clean_path;
    if (matchedLocation) {
        const std::string& locPath = matchedLocation->getLocationPath();
        if (locPath != "/" && resource_path.find(locPath) == 0) {
            resource_path = resource_path.substr(locPath.length());
            if (resource_path.empty())
                resource_path = "/";
            else if (resource_path[0] != '/')
                resource_path = "/" + resource_path;
        }
    }
    std::string file_path;
    std::string autoindex_listing;
    bool isDirectoryRequest = false;
    if (resource_path == "/") {
        file_path = effectiveConfig.documentRoot + "/" + effectiveConfig.indexPath;
        isDirectoryRequest = true;
    } else {
        file_path = effectiveConfig.documentRoot + resource_path;
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            isDirectoryRequest = true;
            if (!resource_path.empty() && resource_path[resource_path.size() - 1] != '/')
                resource_path += "/";
            file_path = effectiveConfig.documentRoot + resource_path + effectiveConfig.indexPath;
        }
    }
    const std::vector<std::string>& allowedMethods = effectiveConfig.allowMethods;
    if (!allowedMethods.empty()) {
        bool isAllowed = false;
        for (size_t i = 0; i < allowedMethods.size(); ++i) {
            if (allowedMethods[i] == req.method) {
                isAllowed = true;
                break;
            }
        }
        if (!isAllowed) {
            Response res;
            res.status_code = 405;
            res.body = "<h1>405 Method Not Allowed</h1>";
            applyCustomErrorPage(res, effectiveConfig);
            std::ostringstream len;
            len << res.body.size();
            res.headers["Content-Length"] = len.str();
            if (res.headers.find("Content-Type") == res.headers.end()) {
                res.headers["Content-Type"] = "text/html";
            }
            client->setResponse(res.toString());
            updatePollEvents(client_fd, POLLOUT);
            return;
        }
    }
    Response res;
    
    // Forbidden zone example (respond 403 for /secret)
    if (matchedLocation && matchedLocation->getLocationPath() == "/secret") {
        Response res;
        res.status_code = 403;
        res.body = "<h1>403 Forbidden</h1>";
        applyCustomErrorPage(res, effectiveConfig);
        std::ostringstream len;
        len << res.body.size();
        res.headers["Content-Length"] = len.str();
        if (res.headers.find("Content-Type") == res.headers.end())
            res.headers["Content-Type"] = "text/html";
        client->setResponse(res.toString());
        updatePollEvents(client_fd, POLLOUT);
        return;
    }
    
    // Determine if the resource targets a CGI script and whether it can run
    bool isCGIRequest = CGIHandler::isCGIScript(file_path);
    bool canExecuteCGI = isCGIRequest && shouldExecuteCGI(file_path, effectiveConfig);

    // Handle different HTTP methods
    if (req.method == "GET") {
        if (isCGIRequest) {
            if (!canExecuteCGI) {
                res.status_code = 500;
                res.body = "<h1>500 Internal Server Error - CGI executor not allowed for this script</h1>";
            } else {
                std::string cgi_executor = effectiveConfig.cgiPass;
                if (cgi_executor.empty()) {
                    cgi_executor = CGIHandler::getInterpreter(file_path);
                }
                if (cgi_executor.empty()) {
                    res.status_code = 500;
                    res.body = "<h1>500 Internal Server Error - No CGI executor configured</h1>";
                } else if (access(cgi_executor.c_str(), X_OK) != 0) {
                    res.status_code = 500;
                    res.body = "<h1>500 Internal Server Error - CGI executor not found or not executable: " + cgi_executor + "</h1>";
                } else {
                    CGIHandler cgi(req, file_path, cgi_executor);
                    std::string cgi_output = cgi.execute();
                    if (!cgi_output.empty()) {
                        // Parse CGI output to separate headers from the body
                        size_t header_end = cgi_output.find("\r\n\r\n");
                        if (header_end == std::string::npos) {
                            header_end = cgi_output.find("\n\n");
                            if (header_end != std::string::npos) {
                                header_end += 2;
                            }
                        } else {
                            header_end += 4;
                        }
                        if (header_end != std::string::npos) {
                            std::string cgi_headers = cgi_output.substr(0, header_end);
                            res.body = cgi_output.substr(header_end);
                            std::istringstream header_stream(cgi_headers);
                            std::string line;
                            while (std::getline(header_stream, line)) {
                                size_t colon_pos = line.find(':');
                                if (colon_pos != std::string::npos) {
                                    std::string key = line.substr(0, colon_pos);
                                    std::string value = line.substr(colon_pos + 1);
                                    while (!value.empty() && value[0] == ' ') value.erase(0, 1);
                                    while (!value.empty() && (value[value.length()-1] == '\r' || value[value.length()-1] == '\n')) value.erase(value.length()-1);
                                    res.headers[key] = value;
                                }
                            }
                        } else {
                            res.body = cgi_output;
                        }
                        res.status_code = 200;
                    } else {
                        res.status_code = 500;
                        res.body = "<h1>500 Internal Server Error - CGI Failed</h1>";
                    }
                }
            }
        } else {
            // Static file or directory
            std::string file_content = readFileNonBlocking(file_path);
            if (!file_content.empty()) {
                res.body = file_content;
                res.status_code = 200;
            } else if (isDirectoryRequest && effectiveConfig.autoIndex) {
                std::string dirPath = effectiveConfig.documentRoot + resource_path;
                DIR* dir = opendir(dirPath.c_str());
                if (dir) {
                    struct dirent* entry;
                    std::ostringstream listing;
                    listing << "<html><body><h1>Index of " << resource_path << "</h1><ul>";
                    while ((entry = readdir(dir)) != NULL) {
                        std::string name(entry->d_name);
                        if (name == ".")
                            continue;
                        std::string href = resource_path + name;
                        listing << "<li><a href=\"" << href << "\">" << name << "</a></li>";
                    }
                    listing << "</ul></body></html>";
                    closedir(dir);
                    res.body = listing.str();
                    res.status_code = 200;
                    if (path_ends_with_exact(file_path, effectiveConfig.uploadStore, effectiveConfig.indexPath)) {
                        res.status_code = 403;
                        res.body = "<h1>403 Forbidden - Directory listing not allowed in upload directory</h1>";
                    }
                } else {
                    res.status_code = 500;
                    res.body = "<h1>500 Internal Server Error</h1>";
                }
            } else {
                res.status_code = 404;
                res.body = "<h1>404 Not Found</h1>";
            }
        }
    }
    else if (req.method == "POST") {

        if (isCGIRequest) {
            if (!canExecuteCGI) {
                res.status_code = 500;
                res.body = "<h1>500 Internal Server Error - CGI executor not allowed for this script</h1>";
            } else {
                std::string cgi_executor = effectiveConfig.cgiPass;
                if (cgi_executor.empty()) {
                    cgi_executor = CGIHandler::getInterpreter(file_path);
                }
                if (cgi_executor.empty()) {
                    res.status_code = 500;
                    res.body = "<h1>500 Internal Server Error - No CGI executor configured</h1>";
                } else if (access(cgi_executor.c_str(), X_OK) != 0) {
                    res.status_code = 500;
                    res.body = "<h1>500 Internal Server Error - CGI executor not found or not executable: " + cgi_executor + "</h1>";
                } else {
                    CGIHandler cgi(req, file_path, cgi_executor);
                    std::string cgi_output = cgi.execute();
                    if (!cgi_output.empty()) {
                        size_t header_end = cgi_output.find("\r\n\r\n");
                        if (header_end == std::string::npos) {
                            header_end = cgi_output.find("\n\n");
                            if (header_end != std::string::npos)
                                header_end += 2;
                        } else {
                            header_end += 4;
                        }
                        if (header_end != std::string::npos) {
                            std::string cgi_headers = cgi_output.substr(0, header_end);
                            res.body = cgi_output.substr(header_end);

                            std::istringstream header_stream(cgi_headers);
                            std::string line;
                            while (std::getline(header_stream, line)) {
                                size_t colon_pos = line.find(':');
                                if (colon_pos != std::string::npos) {
                                    std::string key = line.substr(0, colon_pos);
                                    std::string value = line.substr(colon_pos + 1);
                                    while (!value.empty() && value[0] == ' ') value.erase(0, 1);
                                    while (!value.empty() && 
                                          (value[value.length()-1] == '\r' || value[value.length()-1] == '\n'))
                                        value.erase(value.length()-1);
                                    res.headers[key] = value;
                                }
                            }
                        } else {
                            res.body = cgi_output;
                        }
                        res.status_code = 200;
                    } else {
                        res.status_code = 500;
                        res.body = "<h1>500 Internal Server Error - CGI Failed</h1>";
                    }
                }
            }
        }
        else if (path_ends_with_exact(file_path, effectiveConfig.uploadStore, effectiveConfig.indexPath) && UploadHandler::isUploadRequest(req, effectiveConfig.uploadEnable)) {
            UploadHandler uploader(req, selectedBlock);
            res = uploader.handle();
        } else if (!effectiveConfig.uploadEnable) {
            res.status_code = 403;
            res.body = "<h1>403 Forbidden - Uploads are disabled</h1>";
        } else {
            res.status_code = 404;
        }
    }
    else if (req.method == "DELETE") {
        if (fileExistsNonBlocking(file_path)) {
            if (file_path.find(effectiveConfig.uploadStore + "/") != std::string::npos) {
                if (remove(file_path.c_str()) == 0) {
                    res.status_code = 200;
                    res.body = "<h1>File deleted with DELETE</h1>";

                } else {
                    res.status_code = 500;
                    res.body = "<h1>404 Intenal Server Error</h1>";
                }
            } else {
                res.status_code = 403;
                res.body = "<h1>403 Forbidden - Directory listing not allowed in upload directory</h1>";
            }
        } else {
            res.status_code = 404;
            res.body = "<h1>404 Not Found</h1>";
        }
    }
    else {
        res.status_code = 405;
        res.body = "<h1>405 Method Not Allowed</h1>";
    }

    applyCustomErrorPage(res, effectiveConfig);

// Set response headers
std::ostringstream oss;
oss << res.body.size();
res.headers["Content-Length"] = oss.str();

// Force correct Content-Type for HTML responses
if (res.headers.find("Content-Type") == res.headers.end()) {
    // If the body contains HTML, force text/html
    if (res.body.find("<html") != std::string::npos || 
        res.body.find("<h1>") != std::string::npos ||
        res.body.find("<!DOCTYPE html>") != std::string::npos ||
        res.body.find("<p>") != std::string::npos) {
        res.headers["Content-Type"] = "text/html; charset=UTF-8";
    }
    else {
        // Try to detect the type from the extension
        std::string guessedType = getContentType(file_path);
        if (guessedType == "application/octet-stream")
            res.headers["Content-Type"] = "text/html; charset=UTF-8";
        else
            res.headers["Content-Type"] = guessedType;
    }
}

// Send response
client->setResponse(res.toString());
updatePollEvents(client_fd, POLLOUT);
}

bool WebServer::shouldExecuteCGI(const std::string& filePath, const EffectiveConfig& config) const {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos)
        return false;

    std::string extension = toLower(filePath.substr(dotPos));
    if (config.cgiPass.empty())
        return false;
    return extensionMatchesExecutor(extension, config.cgiPass);
}

bool WebServer::extensionMatchesExecutor(const std::string& extension, const std::string& executor) const {
    std::string executorLower = toLower(executor);
    if (executorLower.empty())
        return false;

    if (executorLower.find("php") != std::string::npos)
        return extension == ".php";
    if (executorLower.find("python") != std::string::npos)
        return extension == ".py";
    if (executorLower.find("perl") != std::string::npos)
        return extension == ".pl";
    if (executorLower.find("ruby") != std::string::npos)
        return extension == ".rb";
    if (executorLower.find("bash") != std::string::npos || executorLower.find("sh") != std::string::npos)
        return (extension == ".sh" || extension == ".cgi");

    return extension == ".cgi";
}

size_t WebServer::parseSize(const std::string& value) const {
    if (value.empty())
        return 0;
    size_t multiplier = 1;
    std::string digits = value;
    char suffix = value[value.size() - 1];
    if (!isdigit(suffix)) {
        digits = value.substr(0, value.size() - 1);
        if (suffix == 'k' || suffix == 'K')
            multiplier = 1024;
        else if (suffix == 'm' || suffix == 'M')
            multiplier = 1024 * 1024;
        else if (suffix == 'g' || suffix == 'G')
            multiplier = 1024 * 1024 * 1024;
    }
    size_t base = static_cast<size_t>(strtoul(digits.c_str(), NULL, 10));
    return base * multiplier;
}
