// vim:ts=4:sw=4:tw=80:et
// debug.h

#ifndef _DEBUG_H
#define _DEBUG_H

#include <Arduino.h>

#define dbg(a)          funcdbg(__FILE__, __LINE__, a)
#define dbgf(...)       funcdbgf(__FILE__, __LINE__, __VA_ARGS__)
#define dbgbin(a, b, c) funcdbgbin(__FILE__, __LINE__, a, b, c)
#define assert(a)       funcassert(__FILE__, __LINE__, a)

void funcdbg(const char* file, long int line, const char *msg);
void funcdbgf(const char* file, long int line, const char *format, ...)
     __attribute__((format(printf, 3, 4)));
void funcdbgbin(const char* file, long int line, const char *prefix,
                const void* data, byte data_len);
void funcassert(const char* file, long int line, bool a);


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

