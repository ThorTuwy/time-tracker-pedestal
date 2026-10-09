#include "WebServer.hpp"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <NameRegisterApi.hpp>

namespace {

AsyncWebServer server(80);

const char* getWebsiteContentType(const String& path) {
    if(path.endsWith(".html")) return "text/html; charset=utf-8";
    if(path.endsWith(".css")) return "text/css; charset=utf-8";
    if(path.endsWith(".js")) return "application/javascript";
    if(path.endsWith(".svg")) return "image/svg+xml";
    if(path.endsWith(".json")) return "application/json";
    if(path.endsWith(".ico")) return "image/x-icon";
    if(path.endsWith(".woff2")) return "font/woff2";
    return "application/octet-stream";
}

void serveCompressedWebsiteFile(AsyncWebServerRequest* request) {
    if(request->method() != HTTP_GET and request->method() != HTTP_HEAD) {
        request->send(405, "text/plain", "Method not allowed");
        return;
    }

    String path = request->url();
    if(path == "/") path = "/index.html";
    if(path.indexOf("..") >= 0 or path.indexOf('\\') >= 0 or
       path.indexOf('%') >= 0) {
        request->send(400, "text/plain", "Invalid path");
        return;
    }

    bool needsGzipEncoding = !path.endsWith(".woff2");
    String filePath = path;
    if(needsGzipEncoding) {
        const AsyncWebHeader* acceptEncodingHeader =
            request->getHeader("Accept-Encoding");
        String acceptedEncodings =
            acceptEncodingHeader == nullptr ? "" : acceptEncodingHeader->value();
        acceptedEncodings.toLowerCase();
        if(acceptedEncodings.indexOf("gzip") < 0) {
            AsyncWebServerResponse* response =
                request->beginResponse(406, "text/plain", "Gzip encoding required");
            response->addHeader("Vary", "Accept-Encoding");
            request->send(response);
            return;
        }

        filePath += ".gz";
    }

    if(!LittleFS.exists(filePath)) {
        request->send(404, "text/plain", "Not found");
        return;
    }

    AsyncWebServerResponse* response =
        request->beginResponse(LittleFS, filePath, getWebsiteContentType(path));
    if(needsGzipEncoding) {
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Vary", "Accept-Encoding");
    }
    request->send(response);
}

void sendBinaryDownload(AsyncWebServerRequest* request, const char* filePath,
                        const char* downloadName) {
    File file = LittleFS.open(filePath, FILE_READ);
    if(!file) {
        request->send(404, "text/plain", "File not found");
        return;
    }

    request->send(request->beginResponse(file, downloadName,
                                         "application/octet-stream", true));
}

}  // namespace

namespace WebServer {

void begin() {
    server.onNotFound(serveCompressedWebsiteFile);
    server.on("/names.bin", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendBinaryDownload(request, "/names.bin", "names.bin");
    });
    server.on("/name.bin", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendBinaryDownload(request, "/names.bin", "names.bin");
    });
    server.on("/history.bin", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendBinaryDownload(request, "/history.bin", "history.bin");
    });
    NameRegisterApi::begin(server);
    server.begin();
}

}  // namespace WebServer
