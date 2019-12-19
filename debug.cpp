// debug.cpp

#include "debug.h"

#include <stdarg.h>

static char buffer[150];

static const char *fullname_to_filename(const char* full) {
    const char *p = full + strlen(full);

    while (p > full) {
        --p;
        if (*p == '/' || *p == '\\')
            break;
    }
    if (*p == '/' || *p == '\\')
        return p + 1;
    else
        return p;
}

void funcdbg(const char* file, const char* msg) {
    Serial.print(fullname_to_filename(file));
    Serial.print("  ");
    Serial.print(msg);
    Serial.print("\n");
}

void funcdbgf(const char* file, const char* format, ...) {
    Serial.print(fullname_to_filename(file));
    Serial.print("  ");
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
    Serial.print("\n");
}
 
static void uint8_to_2char(char c[2], uint8_t v) {
    c[0] = (v >> 4) & 0xF;
    c[1] = v & 0xF;
    for (int i = 0; i < 2; ++i) {
        if (c[i] <= 9) {
            c[i] = '0' + c[i];
        } else {
            c[i] = 'A' + c[i] - 10;
        }
    }
}

void bin_to_hex_string(char *buf, byte buf_len, const void *data, byte data_len) {
    char c[2];

    byte idx = 0;
    for (byte i = 0; i < data_len; ++i) {
        if ((idx + 2) > (buf_len - 1))
            break;
        uint8_to_2char(c, ((char *)data)[i]);
        buf[idx] = c[0];
        buf[idx + 1] = c[1];
        buf[idx + 2] = ' ';
        idx += 3;
    }
    if (idx >= 1)
        --idx;
    buf[idx] = '\0';
}

EventTimer::EventTimer(): next(0), list_locked(false) {
    for (byte i = 0; i < sizeof(list) / sizeof(*list); ++i) {
        list[i].ev = 0;
    }
    for (byte i = 0; i < sizeof(strings) / sizeof(*strings); ++i) {
        strings[i] = NULL;
    }
}

void EventTimer::ev_set_1_string(const byte ev, const char *str) {
    if (ev > sizeof(strings) / sizeof(*strings)) {
        return;
    }
    strings[ev] = str;
}

void EventTimer::ev_set_all_strings(const char **strings, byte nb_strings) {
    for (byte i = 1; i < nb_strings; ++i) {
        ev_set_1_string(i, strings[i]);
    }
}

bool EventTimer::lock_ev_list() {
    bool r = false;

    noInterrupts();
    if (!list_locked) {
        list_locked = true;
        r = true;
    }
    interrupts();

    return r;
}

void EventTimer::unlock_ev_list() {
    list_locked = false;
}

void EventTimer::ev_reg(const byte ev, const unsigned long t) {

    if (!lock_ev_list())
        return;

    list[next].ev = ev;
    list[next].t = t;

    next = (next + 1) % (sizeof(list) / sizeof(*list));

    unlock_ev_list();
}

void EventTimer::serial_print_padded(const unsigned long n, byte w) {
    char spaces[16];
    strncpy(spaces, "               ", sizeof(spaces));
    spaces[sizeof(spaces) - 1] = '\0';

    unsigned long tmp = n;
    byte log10 = 0;
    while (tmp >= 10) {
        tmp /= 10;
        ++log10;
    }
    byte pad = (log10 > w ? 0 : w - log10);
    if (pad > strlen(spaces))
        pad = strlen(spaces);
    spaces[pad] = '\0';
    Serial.print(spaces);
    Serial.print(n);
}

void EventTimer::ev_print() {
    const char* sep_string = "------------+------------+----------\n";
    if (!lock_ev_list())
        return;

    Serial.print(sep_string);

    byte i = next;
    bool first_line = true;
    unsigned long last_t;
    do {
        if (list[i].ev != 0) {
            byte ev = list[i].ev;
            unsigned long t = list[i].t;
            serial_print_padded(t, 10);
            if (!first_line) {
                Serial.print(" |");
                serial_print_padded(t - last_t, 10);
            } else {
                Serial.print(" |           ");
            }

            Serial.print(" | ");
            if (strings[ev]) {
                Serial.print(strings[ev]);
            } else {
                Serial.print("[");
                Serial.print(ev);
                Serial.print("]");
            }

            Serial.print("\n");

            last_t = t;
            first_line = false;
        }
        i = (i + 1) % (sizeof(list) / sizeof(*list));
    } while (i != next);

    Serial.print(sep_string);

    for (byte i = 0; i < sizeof(list) / sizeof(*list); ++i) {
        list[i].ev = 0;
    }

    unlock_ev_list();
}

void EventTimer::ev_print_by_period(const unsigned long period_us) {
    static unsigned long last_t = 0;
    unsigned long t = micros();
    if (t - last_t > period_us) {
        ev_print();
        last_t = t;
    }
}

