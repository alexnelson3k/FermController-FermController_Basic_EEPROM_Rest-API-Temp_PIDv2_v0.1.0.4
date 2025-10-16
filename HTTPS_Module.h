#ifndef HTTPS_Module_h
#define HTTPS_Module_h

#include <esp_https_server.h>
#include <cJSON.h>
#include "Param_helpers.h"
#include "Param_types.h"

class HTTPSModule {
  public:
    HTTPSModule();
    bool begin();
    void handleClient();
    
  private:
    httpd_handle_t server = nullptr;
    
    // Only needed URIs
    httpd_uri_t set_uri;
    httpd_uri_t config_uri;
    
    // Helper methods
    bool is_authorized(httpd_req_t *req);
    void send_unauthorized(httpd_req_t *req);
    
    // Handler methods
    static esp_err_t set_post_handler(httpd_req_t *req);
    static esp_err_t config_get_handler(httpd_req_t *req);
};

extern HTTPSModule httpsModule;

#endif