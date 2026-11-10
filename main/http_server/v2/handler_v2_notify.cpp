#include "handler_v2_notify.h"

#include "esp_http_server.h"
#include "esp_log.h"

#include "ArduinoJson.h"
#include "psram_allocator.h"
#include "global_state.h"
#include "http_cors.h"
#include "http_utils.h"
#include "../displays/ui_ipc.h"

static const char *TAG = "http_v2_notify";

// POST /api/v2/notify
// {"title": "NEW JIRA ISSUE", "key": "EURSUP-5476", "body": "Mensajes Push sin estado", "durationMs": 30000}
// Wakes the display and shows the text. Source agnostic: the caller decides what the lines mean.
esp_err_t POST_V2_notify(httpd_req_t *req)
{
    // close connection when out of scope
    ConGuard g(http_server, req);

    if (is_network_allowed(req) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    }

    if (set_cors_headers(req) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (validateOTP(req) != ESP_OK) {
        return ESP_FAIL;
    }

    PSRAMAllocator allocator;
    JsonDocument doc(&allocator);

    esp_err_t err = getJsonData(req, doc);
    if (err != ESP_OK) {
        return err;
    }

    const char *title = doc["title"].is<const char *>() ? doc["title"].as<const char *>() : nullptr;
    const char *key = doc["key"].is<const char *>() ? doc["key"].as<const char *>() : nullptr;
    const char *body = doc["body"].is<const char *>() ? doc["body"].as<const char *>() : nullptr;

    const bool empty = !(title && *title) && !(key && *key) && !(body && *body);
    if (empty) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "title, key or body required");
    }

    uint32_t duration_ms = 0; // 0 = default
    if (doc["durationMs"].is<uint32_t>()) {
        duration_ms = doc["durationMs"].as<uint32_t>();
    }

    if (!ui_send_notification(title, key, body, duration_ms)) {
        ESP_LOGW(TAG, "ui queue full, notification dropped");
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "display queue full");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "notification queued");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"queued\":true}");
    return ESP_OK;
}
