#include "NameRegisterApi.hpp"

#include <NameRegister.hpp>

#include <stdlib.h>
#include <string.h>

namespace {

struct RegisterUpload {
    uint8_t* data;
    size_t total;
    size_t received;
    bool failed;
};

void clearRegisterUpload(AsyncWebServerRequest* request) {
    RegisterUpload* upload =
        static_cast<RegisterUpload*>(request->_tempObject);
    if(upload != nullptr) {
        free(upload->data);
        free(upload);
        request->_tempObject = nullptr;
    }
}

}  // namespace

namespace NameRegisterApi {

void begin(AsyncWebServer& server) {
    server.on(
        "/api/replaceRegister", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            size_t bodySize = request->contentLength();
            if(bodySize % sizeof(ActivityNameRecord) != 0) {
                clearRegisterUpload(request);
                request->send(400, "text/plain", "Invalid register body size");
                return;
            }

            RegisterUpload* upload =
                static_cast<RegisterUpload*>(request->_tempObject);
            if(bodySize != 0 and
               (upload == nullptr or upload->failed or
                upload->total != bodySize or upload->received != bodySize)) {
                bool uploadFailed = upload != nullptr and upload->failed;
                clearRegisterUpload(request);
                request->send(uploadFailed ? 413 : 400, "text/plain",
                              "Could not receive register body");
                return;
            }

            const uint8_t* body = upload == nullptr ? nullptr : upload->data;
            bool replaced = NameRegister::replaceRegister(body, bodySize);
            clearRegisterUpload(request);
            if(!replaced) {
                request->send(500, "text/plain", "Could not replace register");
                return;
            }
            request->send(200, "text/plain", "Register replaced");
        },
        nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len,
           size_t index, size_t total) {
            RegisterUpload* upload =
                static_cast<RegisterUpload*>(request->_tempObject);
            if(index == 0) {
                clearRegisterUpload(request);
                upload = static_cast<RegisterUpload*>(malloc(sizeof(RegisterUpload)));
                if(upload == nullptr) return;

                upload->data = total == 0 ? nullptr :
                    static_cast<uint8_t*>(malloc(total));
                upload->total = total;
                upload->received = 0;
                upload->failed = total != 0 and upload->data == nullptr;
                request->_tempObject = upload;
            }

            if(upload == nullptr or upload->failed) return;
            if(total != upload->total or index != upload->received or
               index > upload->total or len > upload->total - index) {
                upload->failed = true;
                return;
            }

            if(len != 0) memcpy(upload->data + index, data, len);
            upload->received += len;
        });
}

}  // namespace NameRegisterApi
