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


// The display fonts only cover ASCII. Copy an UTF-8 string into dst, mapping
// Latin-1 letters to their base letter and everything else to '?'.
static void copy_ascii(char *dst, size_t dst_len, const char *src)
{
    // base letters for U+00C0..U+00FF
    static const char latin1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
    size_t o = 0;
    if (!src) {
        src = "";
    }
    const unsigned char *s = (const unsigned char *) src;
    while (*s && o + 1 < dst_len) {
        unsigned char c = *s;
        if (c < 0x80) {
            // control characters (incl. newlines) become spaces
            dst[o++] = (c < 0x20 || c == 0x7f) ? ' ' : (char) c;
            s++;
            continue;
        }
        // decode one UTF-8 sequence
        uint32_t cp = 0;
        int extra = 0;
        if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { s++; dst[o++] = '?'; continue; }
        s++;
        bool valid = true;
        for (int i = 0; i < extra; i++) {
            if ((*s & 0xC0) != 0x80) { valid = false; break; }
            cp = (cp << 6) | (*s & 0x3F);
            s++;
        }
        if (!valid) {
            dst[o++] = '?';
        } else if (cp >= 0xC0 && cp <= 0xFF) {
            dst[o++] = latin1[cp - 0xC0];
        } else if (cp == 0xA1) {
            dst[o++] = '!';
        } else if (cp == 0xBF) {
            dst[o++] = '?';
        } else if (cp == 0x2018 || cp == 0x2019) {
            dst[o++] = '\'';
        } else if (cp == 0x201C || cp == 0x201D || cp == 0xAB || cp == 0xBB) {
            dst[o++] = '"';
        } else if (cp == 0x2013 || cp == 0x2014) {
            dst[o++] = '-';
        } else if (cp == 0xA0) {
            dst[o++] = ' ';
        } else {
            dst[o++] = '?';
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
    copy_ascii(n->title, sizeof(n->title), title);
    copy_ascii(n->key, sizeof(n->key), key);
    copy_ascii(n->body, sizeof(n->body), body);

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
