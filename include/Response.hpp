#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>

class Response {
public:
    int status_code;
    std::map<std::string, std::string> headers;
    std::string body;

    Response();
    std::string toString() const;

private:
    std::string getStatusMessage(int code) const;
};

#endif
