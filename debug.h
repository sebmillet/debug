// debug.h

#ifndef _DEBUG_H
#define _DEBUG_H

#include <Arduino.h>

#define dbginit() Serial.begin(9600)
#define dbg(a)    funcdbg(__FILE__, a)
#define dbgf(...) funcdbgf(__FILE__, __VA_ARGS__)
//#define dbgprint(a) Serial.print(a)
//#define dbgprintln(a) { Serial.print(a); Serial.print("\n"); }

void funcdbg(const char* file, const char *msg);
void funcdbgf(const char* file, const char *format, ...)
     __attribute__((format(printf, 2, 3)));

void bin_to_hex_string(char *buf, byte buf_len, const void *data, byte data_len);


//
// EVENTS TIMING MANAGEMENT
//

typedef struct {
    byte ev;
    unsigned long t;
} ev_t;

#define EV_MAX_TYPES   20
#define EV_BUFFER_SIZE 60

class EventTimer {

    private:
        const char* strings[EV_MAX_TYPES];
        ev_t list[EV_BUFFER_SIZE];
        byte next;
        bool list_locked;

        void serial_print_padded(const unsigned long n, byte w);
        bool lock_ev_list();
        void unlock_ev_list();

    public:
        EventTimer();
        void ev_set_1_string(const byte ev, const char *str);
        void ev_set_all_strings(const char *strings[], byte nb_strings);
        void ev_reg(const byte ev, const unsigned long t);
        void ev_print();
        void ev_print_by_period(const unsigned long period_us);

};

#endif

