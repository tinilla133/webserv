#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include <Request.hpp>
#include <Response.hpp>
#include <ServerBlockConfig.hpp>

#include <string>

class UploadHandler {
private:
    const Request& req;
    const ServerBlock& serverBlock;

    std::string extractBoundary() const;
    std::string saveFile(const std::string& filename, const std::string& content) const;
    std::string generateHTMLSuccess(const std::string& filename, size_t size) const;
    std::string generateHTMLError(const std::string& msg) const;

public:
    UploadHandler(const Request& request, const ServerBlock& cfg);
    Response handle();

    static bool isUploadRequest(const Request& req, bool isUploadEnabled);
};

#endif
