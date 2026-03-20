#pragma once

#define CLEAR(x) memset(&(x), 0, sizeof(x))

class Flag {
public:
    enum states {
        UNDEF,
        CLR,
        SET,
        FALL,
        RISE
    };
    enum states v = Flag::UNDEF;
};

#define DEG2RAD(deg) ((deg) * M_PI / 180)
#define RAD2DEG(rad) ((rad) / M_PI * 180)
#define RAD_NORM(rad) ( \
((rad) >  M_PI) ? (rad) - 2 * M_PI : \
((rad) < -M_PI) ? 2 * M_PI + (rad) : \
 (rad) )

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#endif

#define PRESC(_x) static int presc##__COUNTER__##__func__ = 0;\
    if (0 == (presc##__COUNTER__##__func__++ & _x))
