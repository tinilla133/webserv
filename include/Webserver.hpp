 #ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <FileParsing.hpp>
#include <Client.hpp>
#include <Config.hpp>
#include <Server.hpp>
#include <Request.hpp>
#include <LocationBlockConfig.hpp>
#include <Response.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>


#if 1
#define ERR_PRINT(msg) std::cerr << "[ERROR] " << __FILE__ << ":" << __LINE__ \
    << " (" << __func__ << ") - " << msg << std::endl;
#else
#define ERR_PRINT(...)
#endif

#if PRINT_DEBUG
#define DBG_PRINT(msg) std::cout << "[DEBUG] " << __FILE__ << ":" << __LINE__ \
    << " (" << __func__ << ") - " << msg << std::endl;
#else
#define DBG_PRINT(...)
#endif

#define INF_PRINT(msg) std::cout << "[INFO] " << __FILE__ << ":" << __LINE__ \
    << " (" << __func__ << ") - " << msg << std::endl;

#define VER_PRINT(msg) std::cout << msg << std::endl;

// Structure for effective configuration (server + location merged)
struct EffectiveConfig {
    std::string documentRoot;
    std::string errorPageRoot;
    std::string serverRoot;
    std::string indexPath;
    std::vector<std::string> allowMethods;
    std::string clientMaxBodySize;
    bool autoIndex;
    bool uploadEnable;
    std::string uploadStore;
    std::string cgiPass;
    std::map<int, std::string> errorPageMap;
};

class WebServer {
    private:
        Config				        serverConfig;
        std::vector<pollfd>	        fds;
        std::map<int, Client*>      clients;
        std::vector<Server*>        servers;
        std::map<int, int>          fdToServerIndex;  // listener_fd -> server index
        std::map<int, int>          clientToListener; // client_fd -> listener_fd

        bool shouldExecuteCGI(const std::string& filePath, const EffectiveConfig& config) const;
        bool extensionMatchesExecutor(const std::string& extension, const std::string& executor) const;

        WebServer(const WebServer&);
        WebServer& operator=(const WebServer&);

    public:
        WebServer();
        ~WebServer();

        bool 			loadConfig(const std::string& path);
        const Config&	getConfig() const;
        void 			setConfig(const Config& cfg);

        void            handleNewConnection(int listener_fd);
        void            handleClientData(int client_fd);
        void            handleClientSend(int client_fd);
        void            cleanupClient(int client_fd);
        void            updatePollEvents(int client_fd, short events);
        void            processClientRequest(int client_fd);
        Config&         getServerConfig(void);
        size_t          parseSize(const std::string& value) const;

        bool initMultipleServers();
        void run();
        bool isListenerSocket(int fd) const;
        const ServerBlock& selectServerBlock(int listener_fd, const Request& req) const;
        EffectiveConfig mergeConfigurations(const ServerBlock& server, const LocationBlock* location) const;
        void applyCustomErrorPage(Response& res, const EffectiveConfig& config) const;
};

#endif
