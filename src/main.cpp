// /* ************************************************************************** */
// /*                                                                            */
// /*                                                        :::      ::::::::   */
// /*   main.cpp                                           :+:      :+:    :+:   */
// /*                                                    +:+ +:+         +:+     */
// /*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
// /*                                                +#+#+#+#+#+   +#+           */
// /*   Created: 2025/07/27 14:02:14 by fvizcaya          #+#    #+#             */
// /*   Updated: 2025/11/07 11:53:42 by aurodrig         ###   ########.fr       */
// /*                                                                            */
// /* ************************************************************************** */


#include <Config.hpp>
#include <FileParsing.hpp>
#include <Webserver.hpp>

int main(int argc, char **argv) {

    std::string config_path;
    WebServer   server;
    
    if (argc > 1)
        config_path = argv[1];
    else
        config_path = "config/default.conf";

    try {
        configFileParser parser;
		parser.setFilePath(config_path);
		parser.setConfigObject(server.getServerConfig());
        if (!parser.parseFile()) {
            ERR_PRINT("Error parsing configuration file.");
            return 1;
        }
        
        server.getServerConfig().supressDummyServerBlocks();
        
		server.getServerConfig().printConfig();
    } catch (const std::exception& e) {
        ERR_PRINT("Exception: " + std::string(e.what()));
        return (1);
    }
    
    if (server.getServerConfig().getNumServerBlocks() == 0) {
        ERR_PRINT("No server blocks configured.");
        return 1;
    }
    
    INF_PRINT("Starting initMultipleServers() for all configured servers");
    if (!server.initMultipleServers()) {
        ERR_PRINT("Server initialization failed. Exiting cleanly.");
        return (1);
    }

    server.run();
    std::cout << "Server setup complete. Exiting." << std::endl;
    return 0;
}
