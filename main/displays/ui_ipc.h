// ui_ipc.h
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum
{
    UI_CMD_SHOW_QR,
    UI_CMD_HIDE_QR,
    UI_CMD_IDENTIFY,      // blink display for identify; param = duration_ms (0 = default 60s)
    UI_CMD_NOTIFICATION,  // wake display and show a text notification; payload = ui_notification_t* (heap), param = duration_ms
} ui_cmd_t;

typedef struct
{
    ui_cmd_t type;
    void *payload;  // e.g., char* uri (heap)
    uint32_t param; // e.g., max_px or timeout
} ui_msg_t;

#define UI_NOTIFY_TITLE_LEN 32
#define UI_NOTIFY_KEY_LEN   24
#define UI_NOTIFY_BODY_LEN  160
#define UI_NOTIFY_DEFAULT_MS 30000
#define UI_NOTIFY_MIN_MS     1000
#define UI_NOTIFY_MAX_MS     600000

// generic notification, not tied to any source (Jira, GitHub, ...)
typedef struct
{
    char title[UI_NOTIFY_TITLE_LEN];
    char key[UI_NOTIFY_KEY_LEN];
    char body[UI_NOTIFY_BODY_LEN];
} ui_notification_t;

extern QueueHandle_t g_ui_queue;

void ui_ipc_init(void);
bool ui_send_show_qr();
bool ui_send_hide_qr();
bool ui_send_identify(uint32_t duration_ms);  // 0 = use default (60s)
// strings are UTF-8 and may be NULL; they are copied, truncated and reduced to ASCII. 0 = default duration (30s)
bool ui_send_notification(const char *title, const char *key, const char *body, uint32_t duration_ms);
