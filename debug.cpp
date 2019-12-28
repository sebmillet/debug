// vim:ts=4:sw=4:tw=80:et
// debug.cpp

#include "debug.h"

#include <stdarg.h>

// What items to include in the debug lines:
#define PRINT_TIMESTAMP
#define PRINT_FILENAME
#define REMOVE_FILE_EXTENSION
#define DEFAULT_PRINT_LINE_NUMBER false

static char buffer[150];
static char progmem_reading_buffer[100];

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
 
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

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
static void print_prefix_of_debug_line(const char* full_name, long int line,
              bool print_line_number = DEFAULT_PRINT_LINE_NUMBER) {

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

    if (print_line_number) {
        char append[12];
        snprintf(append, sizeof(append), ":%li", line);
        strncat(modifiable_string, append, sizeof(modifiable_string));
        modifiable_string[sizeof(modifiable_string) - 1] = '\0';
    }

    Serial.print(modifiable_string);
    print_spaces(PRINT_PADDED_FILENAME_WIDTH - strlen(modifiable_string));
    Serial.print("  ");

#endif // PRINT_FILENAME

}

void dbgfunc(const char* file, long int line, const char* progmem_str) {

    funcdbginit();

    strcpy_P(progmem_reading_buffer, progmem_str);
    print_prefix_of_debug_line(file, line);
    Serial.print(progmem_reading_buffer);
    Serial.print("\n");
}

void dbgffunc(const char* file, long int line, const char* progmem_fmt, ...) {

    funcdbginit();

    strcpy_P(progmem_reading_buffer, progmem_fmt);
    print_prefix_of_debug_line(file, line);
    va_list args;

    // FIXME
#pragma GCC diagnostic ignored "-Wvarargs"
    va_start(args, progmem_reading_buffer);

    vsnprintf(buffer, sizeof(buffer), progmem_reading_buffer, args);
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

void dbgbinfunc(const char* file, long int line, const char *prefix,
                const void* data, byte data_len) {

    funcdbginit();

    print_prefix_of_debug_line(file, line);
    Serial.print(prefix);

    char small_buf[20];
    snprintf(small_buf, sizeof(small_buf), "l=%i:  ", data_len);
    Serial.print(small_buf);
    bin_to_hex_string(buffer, sizeof(buffer), data, data_len);
    Serial.print(buffer);
    Serial.print("\n");
}

void assertfunc(const char* file, long int line, bool a) {

    if (a) {
        return;
    }

    funcdbginit();

    print_prefix_of_debug_line(file, line, true);
    Serial.print("assert condition not met");
    Serial.print("\n");
    delay(200);

    while (1)
        ;

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

void EventTimer::ev_set_all_strings(const char* const* strings, byte nb) {
    for (byte i = 1; i < nb; ++i) {
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

void EventTimer::ev_reg(byte ev, unsigned long t) {

    if (!lock_ev_list())
        return;

    list[next].ev = ev;
    if (t == 4294967295) {
        t = millis();
    }
    list[next].t = t;

    next = (next + 1) % (sizeof(list) / sizeof(*list));

    unlock_ev_list();
}

void EventTimer::serial_print_padded(const unsigned long n) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%7lu.%03u",
      n / 1000, (unsigned int)(n % 1000));
    Serial.print(buffer);
}

const char sep_string[] PROGMEM = "------------+------------+----------\n";

void print_progmem_string(const char *s) {
    char buffer[EV_STRING_MAX_LENGTH + 1];

    strcpy_P(buffer, s);
    Serial.print(buffer);
}

void EventTimer::ev_print() {
    if (!lock_ev_list())
        return;

    funcdbginit();

    byte i = next;
    int count = 0;
    do {
        if (list[i].ev != 0) {
            ++count;
        }
        i = (i + 1) % (sizeof(list) / sizeof(*list));
    } while (i != next);

    if (!count) {
        Serial.print(".\n");
        unlock_ev_list();
        return;
    }

    print_progmem_string(sep_string);

    i = next;
    bool first_line = true;
    unsigned long last_t;
    do {
        if (list[i].ev != 0) {
            byte ev = list[i].ev;
            unsigned long t = list[i].t;
            serial_print_padded(t);
            if (!first_line) {
                Serial.print(" |");
                serial_print_padded(t - last_t);
            } else {
                Serial.print(" |           ");
            }

            Serial.print(" | ");
            if (strings[ev]) {
                print_progmem_string(strings[ev]);
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

    print_progmem_string(sep_string);

    for (byte i = 0; i < sizeof(list) / sizeof(*list); ++i) {
        list[i].ev = 0;
    }

    unlock_ev_list();
}

void EventTimer::ev_print_by_period(const unsigned long period_ms) {
    static unsigned long last_t = 0;
    unsigned long t = millis();
    if (t - last_t > period_ms) {
        ev_print();
        last_t = t;
    }
}

