#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "ui_ipc.h"

QueueHandle_t g_ui_queue = NULL;

void ui_ipc_init(void) {
    g_ui_queue = xQueueCreate(/*len*/ 8, sizeof(ui_msg_t));
}

bool ui_send_show_qr() {
    ui_msg_t m = { .type = UI_CMD_SHOW_QR, .payload = nullptr };
    if (xQueueSend(g_ui_queue, &m, 0) != pdTRUE) {
        return false;
    }
    return true;
}

bool ui_send_hide_qr() {
    ui_msg_t m = { .type = UI_CMD_HIDE_QR, .payload = nullptr };
    if (xQueueSend(g_ui_queue, &m, 0) != pdTRUE) {
        return false;
    }
    return true;
}

bool ui_send_identify(uint32_t duration_ms) {
    ui_msg_t m = { .type = UI_CMD_IDENTIFY, .payload = nullptr,
                   .param = duration_ms ? duration_ms : 60000 };
    if (xQueueSend(g_ui_queue, &m, 0) != pdTRUE) {
        return false;
    }
    return true;
}


// The notification fonts cover ASCII and Latin-1 (U+00A0..U+00FF). Copy an UTF-8
// string into dst keeping those, mapping some typographic characters to their
// plain form and everything else to '?'. Never cuts inside a multi byte character.
static void copy_text(char *dst, size_t dst_len, const char *src)
{
    size_t o = 0;
    if (!src) {
        src = "";
    }
    const unsigned char *s = (const unsigned char *) src;
    while (*s) {
        unsigned char c = *s;
        uint32_t cp = 0;
        if (c < 0x80) {
            // control characters (incl. newlines) become spaces
            cp = (c < 0x20 || c == 0x7f) ? ' ' : c;
            s++;
        } else {
            // decode one UTF-8 sequence
            int extra = 0;
            if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
            else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
            else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
            else { extra = -1; }
            s++;
            bool valid = extra > 0;
            for (int i = 0; i < extra; i++) {
                if ((*s & 0xC0) != 0x80) { valid = false; break; }
                cp = (cp << 6) | (*s & 0x3F);
                s++;
            }
            if (!valid) {
                cp = '?';
            } else if (cp >= 0xA0 && cp <= 0xFF) {
                // kept as is
            } else if (cp == 0x2018 || cp == 0x2019) {
                cp = '\'';
            } else if (cp == 0x201C || cp == 0x201D) {
                cp = '"';
            } else if (cp == 0x2013 || cp == 0x2014) {
                cp = '-';
            } else if (cp == 0x2026) {
                cp = '.';
            } else {
                cp = '?';
            }
        }

        if (cp < 0x80) {
            if (o + 1 >= dst_len) {
                break;
            }
            dst[o++] = (char) cp;
        } else {
            // two byte UTF-8, cp is in U+00A0..U+00FF
            if (o + 2 >= dst_len) {
                break;
            }
            dst[o++] = (char) (0xC0 | (cp >> 6));
            dst[o++] = (char) (0x80 | (cp & 0x3F));
        }
    }
    dst[o] = 0;
}

bool ui_send_notification(const char *title, const char *key, const char *body, uint32_t duration_ms) {
    if (!g_ui_queue) {
        return false;
    }
    ui_notification_t *n = (ui_notification_t *) calloc(1, sizeof(ui_notification_t));
    if (!n) {
        return false;
    }
    copy_text(n->title, sizeof(n->title), title);
    copy_text(n->key, sizeof(n->key), key);
    copy_text(n->body, sizeof(n->body), body);

    if (!duration_ms) {
        duration_ms = UI_NOTIFY_DEFAULT_MS;
    }
    if (duration_ms < UI_NOTIFY_MIN_MS) {
        duration_ms = UI_NOTIFY_MIN_MS;
    }
    if (duration_ms > UI_NOTIFY_MAX_MS) {
        duration_ms = UI_NOTIFY_MAX_MS;
    }

    ui_msg_t m = { .type = UI_CMD_NOTIFICATION, .payload = n, .param = duration_ms };
    if (xQueueSend(g_ui_queue, &m, 0) != pdTRUE) {
        free(n);
        return false;
    }
    return true;
}
