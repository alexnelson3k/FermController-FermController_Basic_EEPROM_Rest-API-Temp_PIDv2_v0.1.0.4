#include "HTTPS_Module.h"
#include "cert.h"

HTTPSModule::HTTPSModule() {
    // Initialize URI structures
    memset(&set_uri, 0, sizeof(set_uri));
    set_uri.uri       = "/api/set";
    set_uri.method    = HTTP_POST;
    set_uri.handler   = set_post_handler;
    set_uri.user_ctx  = this;

    memset(&config_uri, 0, sizeof(config_uri));
    config_uri.uri    = "/api/config";
    config_uri.method = HTTP_GET;
    config_uri.handler = config_get_handler;
    config_uri.user_ctx = this;
}

bool HTTPSModule::begin() {
    Serial.println("Initializing HTTPS API module...");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.servercert     = (const uint8_t*)server_cert;
    conf.servercert_len = strlen_P(server_cert) + 1;
    conf.prvtkey_pem    = (const uint8_t*)server_key;
    conf.prvtkey_len    = strlen_P(server_key) + 1;

    if (httpd_ssl_start(&server, &conf) == ESP_OK) {
        httpd_register_uri_handler(server, &set_uri);
        httpd_register_uri_handler(server, &config_uri);
        Serial.println("HTTPS server started successfully");
        return true;
    } else {
        Serial.println("Failed to start HTTPS server");
        return false;
    }
}

bool HTTPSModule::is_authorized(httpd_req_t *req) {
    char auth_header[150];
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth_header, sizeof(auth_header)) == ESP_OK) {
        // Check against API token from parameter system
        if (strstr(auth_header, getParamString(PARAM_API_TOKEN))) {
            return true;
        }
    }
    return false;
}

void HTTPSModule::send_unauthorized(httpd_req_t *req) {
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"error\":\"Unauthorized\"}");
}

// GET /api/config - get all parameters with API_ACCESS flag
esp_err_t HTTPSModule::config_get_handler(httpd_req_t *req) {
    HTTPSModule* instance = (HTTPSModule*)req->user_ctx;
    if (!instance->is_authorized(req)) {
        instance->send_unauthorized(req);
        return ESP_OK;
    }

    cJSON *root = cJSON_CreateObject();
    
    // Add all parameters with API_ACCESS flag
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (system_params[i].flags & API_ACCESS) {
            if (system_params[i].flags & SECURED_VALUE) {
                cJSON_AddStringToObject(root, system_params[i].name, "******");
            } else {
                switch (system_params[i].type) {
                    case TYPE_FLOAT:
                        cJSON_AddNumberToObject(root, system_params[i].name, system_params[i].number.value);
                        break;
                    case TYPE_UINT8:
                        cJSON_AddNumberToObject(root, system_params[i].name, system_params[i].uint8.value);
                        break;
                    case TYPE_UINT16:
                        cJSON_AddNumberToObject(root, system_params[i].name, system_params[i].uint16.value);
                        break;
                    case TYPE_BOOL:
                        cJSON_AddBoolToObject(root, system_params[i].name, system_params[i].boolean.value);
                        break;
                    case TYPE_STRING:
                        cJSON_AddStringToObject(root, system_params[i].name, system_params[i].string.value);
                        break;
                }
            }
        }
    }

    const char* response = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);

    free((void*)response);
    cJSON_Delete(root);
    return ESP_OK;
}

// POST /api/set - set parameters
esp_err_t HTTPSModule::set_post_handler(httpd_req_t *req) {
    HTTPSModule* instance = (HTTPSModule*)req->user_ctx;
    if (!instance->is_authorized(req)) {
        instance->send_unauthorized(req);
        return ESP_OK;
    }

    char buf[512];
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) return ESP_FAIL;
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_OK;
    }

    int successCount = 0;
    int errorCount = 0;
    
    // Process each parameter in JSON
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        const char* paramName = item->string;
        bool success = false;
        
        // Find parameter by name
        for (int i = 0; i < PARAM_COUNT; i++) {
            if (strcmp(paramName, system_params[i].name) == 0 && 
                (system_params[i].flags & API_ACCESS)) {
                
                ConfigParam& param = system_params[i];
                
                // Set value based on type
                switch (param.type) {
                    case TYPE_FLOAT:
                        if (cJSON_IsNumber(item)) {
                            float newValue = item->valuedouble;
                            if (newValue >= param.number.min_value && newValue <= param.number.max_value) {
                                param.number.value = newValue;
                                success = true;
                            }
                        }
                        break;
                    case TYPE_UINT8:
                        if (cJSON_IsNumber(item)) {
                            int newValue = item->valueint;
                            if (newValue >= param.uint8.min_value && newValue <= param.uint8.max_value) {
                                param.uint8.value = newValue;
                                success = true;
                            }
                        }
                        break;
                    case TYPE_BOOL:
                        if (cJSON_IsBool(item)) {
                            param.boolean.value = cJSON_IsTrue(item);
                            success = true;
                        }
                        break;
                    case TYPE_STRING:
                        if (cJSON_IsString(item)) {
                            const char* newValue = item->valuestring;
                            if (strlen(newValue) < param.string.max_size) {
                                strncpy(param.string.value, newValue, param.string.max_size - 1);
                                param.string.value[param.string.max_size - 1] = '\0';
                                success = true;
                            }
                        }
                        break;
                }
                break; // Parameter found, stop searching
            }
        }
        
        if (success) successCount++;
        else errorCount++;
    }

    cJSON_Delete(root);
    
    // Send response
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(response_json, "success", successCount);
    cJSON_AddNumberToObject(response_json, "errors", errorCount);
    cJSON_AddStringToObject(response_json, "message", 
                           successCount > 0 ? "Parameters updated" : "No parameters updated");
    
    const char* response = cJSON_PrintUnformatted(response_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);

    free((void*)response);
    cJSON_Delete(response_json);
    return ESP_OK;
}

void HTTPSModule::handleClient() {
    // HTTPS server runs in background
}

HTTPSModule httpsModule;