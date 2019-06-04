#include <stdlib.h>

class Clock {

public:
    int hour;
    int minute;
    int second;
    
    Clock(int h = 6, int m = 0, int s = 0)
    : hour(h), minute(m), second(s)
    {}

    void jump_in_time(int sec);

    int now();
};