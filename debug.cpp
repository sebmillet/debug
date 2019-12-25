// vim:ts=4:sw=4:tw=80:et
// debug.cpp

#include "debug.h"

#include <stdarg.h>

// What items to include in the debug lines:
#define PRINT_TIMESTAMP
#define PRINT_FILENAME
#define REMOVE_FILE_EXTENSION
//#define PRINT_LINE_NUMBER

static char buffer[150];

void funcdbginit() {
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;

    Serial.begin(9600);
}

static void print_spaces(int nb) {
    char sp[20];
    strncpy(sp, "                    ", sizeof(sp));
    sp[19] = '\0';

    if (nb <= 0)
        return;
    if (nb > (int)strlen(sp))
        nb = (int)strlen(sp);
    sp[nb] = '\0';

    Serial.print(sp);
}

static const byte PRINT_PADDED_FILENAME_WIDTH = 13;
static void print_prefix_of_debug_line(const char* full_name, long int line) {

#ifdef PRINT_TIMESTAMP
    char ts[9];
    unsigned long t = millis();
    snprintf(ts, sizeof(ts), "%3lu.%03lu", t / 1000, t % 1000);
    Serial.print(ts);
    Serial.print("  ");
#endif // PRINT_TIMESTAMP

#ifdef PRINT_FILENAME

    const char *p = full_name + strlen(full_name);
    while (p > full_name) {
        --p;
        if (*p == '/' || *p == '\\')
            break;
    }
    const char* f;
    if (*p == '/' || *p == '\\')
        f = p + 1;
    else
        f = p;

    char modifiable_string[PRINT_PADDED_FILENAME_WIDTH + 1];
    strncpy(modifiable_string, f, sizeof(modifiable_string));
    modifiable_string[sizeof(modifiable_string) - 1] = '\0';

#ifdef REMOVE_FILE_EXTENSION
    char *e = modifiable_string + strlen(modifiable_string);
    while (e > modifiable_string) {
        --e;
        if (*e == '.') {
            *e = '\0';
            break;
        }
    }
#endif // REMOVE_FILE_EXTENSION

#ifdef PRINT_LINE_NUMBER
    char append[12];
    snprintf(append, sizeof(append), ":%i", line);
    strncat(modifiable_string, append, sizeof(modifiable_string));
    modifiable_string[sizeof(modifiable_string) - 1] = '\0';
#endif // PRINT_LINE_NUMBER

    Serial.print(modifiable_string);
    print_spaces(PRINT_PADDED_FILENAME_WIDTH - strlen(modifiable_string));
    Serial.print("  ");

#endif // PRINT_FILENAME

}

void funcdbg(const char* file, long int line, const char* msg) {

    funcdbginit();

    print_prefix_of_debug_line(file, line);
    Serial.print(msg);
    Serial.print("\n");
}

void funcdbgf(const char* file, long int line, const char* format, ...) {

    funcdbginit();

    print_prefix_of_debug_line(file, line);
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

static void bin_to_hex_string(char *buf, byte buf_len,
                              const void *data, byte data_len) {
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

char output[200];
void funcdbgbin(const char* file, long int line, const char *prefix,
                const void* data, byte data_len) {

    funcdbginit();

    bin_to_hex_string(output, sizeof(output), data, data_len);
    print_prefix_of_debug_line(file, line);
    Serial.print(prefix);

    char small_buf[20];
    snprintf(small_buf, sizeof(small_buf), "l=%i:  ", data_len);
    Serial.print(small_buf);
    Serial.print(output);
    Serial.print("\n");
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

