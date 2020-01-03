// vim:ts=4:sw=4:tw=80:et
/*
  debug.h

  Header file of debug.cpp
*/

/*
  Copyright 2020 Sébastien Millet

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses>.
*/

#ifndef _DEBUG_H
#define _DEBUG_H

#include <Arduino.h>

#define EV_STRING_MAX_LENGTH 40

#define dbg(a) \
    { static const char tmp[] PROGMEM = {a}; \
      dbgfunc(__FILE__, __LINE__, tmp); \
    }

#define dbgf(a, ...) \
    { static const char tmp[] PROGMEM = {a}; \
      dbgffunc(__FILE__, __LINE__, tmp, __VA_ARGS__); \
    }

#define dbgbin(a, b, c) dbgbinfunc(__FILE__, __LINE__, a, b, c)
#define assert(a)       assertfunc(__FILE__, __LINE__, a)

void dbgfunc(const char* file, long int line, const char *msg);
void dbgffunc(const char* file, long int line, const char *format, ...)
     __attribute__((format(printf, 3, 4)));
void dbgbinfunc(const char* file, long int line, const char *prefix,
                const void* data, byte data_len);
void assertfunc(const char* file, long int line, bool a);

int freeMemory();

//
// EVENTS TIMING MANAGEMENT
//

typedef struct {
    byte ev;
    unsigned long t;
} ev_t;

#define EV_MAX_TYPES   10
#define EV_BUFFER_SIZE 60

class EventTimer {

    private:
        const char* strings[EV_MAX_TYPES];
        ev_t list[EV_BUFFER_SIZE];
        byte next;
        bool list_locked;

        void serial_print_padded(const unsigned long n);
        bool lock_ev_list();
        void unlock_ev_list();

    public:
        EventTimer();
        void ev_set_1_string(const byte ev, const char *str);
        void ev_set_all_strings(const char* const* strings, byte nb);
        void ev_reg(byte ev, unsigned long t = 4294967295);
        void ev_print();
        void ev_print_by_period(const unsigned long period_ms);

};

#endif

