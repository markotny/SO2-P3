#include <stdlib.h>

class Time {

public:
    int hour;
    int minute;
    int second;
    
    Time(int h = 6, int m = 0, int s = 0)
    : hour(h), minute(m), second(s)
    {}

    void jump_in_time(int sec);

    int now();
};