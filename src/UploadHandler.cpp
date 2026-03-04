/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UploadHandler.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/28 20:18:11 by fmorenil          #+#    #+#             */
/*   Updated: 2025/12/06 16:45:43 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <UploadHandler.hpp>
#include <utils.hpp>
#include <Webserver.hpp>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <dirent.h>


UploadHandler::UploadHandler(const Request& request, const ServerBlock& cfg)
: req(request), serverBlock(cfg) {}

bool UploadHandler::isUploadRequest(const Request& req, bool isUploadEnabled) {
    if (!isUploadEnabled) {
        ERR_PRINT("Upload feature is disabled in server configuration.");
        return false;
    }

    std::map<std::string,std::string>::const_iterator it = req.headers.find("content-type");
    if (it == req.headers.end()) return false;
    return it->second.find("multipart/form-data") != std::string::npos;
}

std::string UploadHandler::extractBoundary() const {
    std::map<std::string,std::string>::const_iterator it = req.headers.find("content-type");
    if (it == req.headers.end()) return "";
    const std::string& contentType = it->second;
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) return "";
    std::string b = contentType.substr(pos + 9);
    // Optional sanitization:
    // - remove quotes if present: boundary="----WebKit..."
    if (!b.empty() && b[0] == '"') {
        size_t endq = b.find('"', 1);
        b = (endq == std::string::npos) ? b.substr(1) : b.substr(1, endq-1);
    }
    while (!b.empty() && (b[b.size()-1] == ';' || b[b.size()-1] == ' ')) b.erase(b.size()-1);
    return "--" + b;
}

std::string UploadHandler::saveFile(const std::string& filename, const std::string& content) const {
    std::string uploadPath = serverBlock.getUploadStore();
    std::string path = uploadPath + "/" + filename;

    if (!writeFileNonBlocking(path, content))
        throw std::runtime_error("Error writing file: " + path);

    return path;
}

std::string UploadHandler::generateHTMLSuccess(const std::string& filename, size_t size) const {
    std::string templatePath = "www/upload_success.html";
    std::string html = readFileNonBlocking(templatePath);

    if (html.empty()) {
        // If the file is not found, use an internal fallback
        std::ostringstream fallback;
        fallback << "<!DOCTYPE html><html><body>"
                 << "<h1>✅ Upload Successful</h1>"
                 << "<p>File saved as: " << filename << "</p>"
                 << "<p>Size: " << size << " bytes</p>"
                 << "</body></html>";
        return fallback.str();
    }

    // Replace placeholders
    size_t pos;
    if ((pos = html.find("{{FILENAME}}")) != std::string::npos)
        html.replace(pos, 12, filename);

    std::ostringstream sizeStr;
    sizeStr << size;
    if ((pos = html.find("{{SIZE}}")) != std::string::npos)
        html.replace(pos, 8, sizeStr.str());

    return html;
}

std::string UploadHandler::generateHTMLError(const std::string& msg) const {
    std::ostringstream html;
    html << "<!DOCTYPE html><html><body><h1>❌ " << msg << "</h1></body></html>";
    return html.str();
}

Response UploadHandler::handle() {
    Response res;
    std::string boundary = extractBoundary();

    if (boundary.empty()) {
        res.status_code = 400;
        res.body = generateHTMLError("Missing boundary");
        return res;
    }

    size_t start = req.body.find(boundary);
    while (start != std::string::npos) {
        size_t end = req.body.find(boundary, start + boundary.size());
        if (end == std::string::npos) break;

        std::string part = req.body.substr(start + boundary.size(),
                                           end - (start + boundary.size()));

        size_t filenamePos = part.find("filename=\"");
        if (filenamePos != std::string::npos) {
            filenamePos += 10;
            size_t filenameEnd = part.find("\"", filenamePos);
            std::string filename = part.substr(filenamePos, filenameEnd - filenamePos);

            size_t dataPos = part.find("\r\n\r\n");
            if (dataPos != std::string::npos) {
                std::string fileContent = part.substr(dataPos + 4);
                if (fileContent.size() >= 2 &&
                    fileContent.substr(fileContent.size() - 2) == "\r\n")
                    fileContent = fileContent.substr(0, fileContent.size() - 2);

                try {
                    saveFile(filename, fileContent);
                    res.status_code = 201;
                    res.body = generateHTMLSuccess(filename, fileContent.size());
                } catch (std::exception &e) {
                    res.status_code = 500;
                    res.body = generateHTMLError(e.what());
                }
            }
        }
        start = end;
    }

    res.headers["Content-Type"] = "text/html; charset=UTF-8";
    std::ostringstream oss;
    oss << res.body.size();
    res.headers["Content-Length"] = oss.str();
    return res;
}
