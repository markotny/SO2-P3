#include <mutex>

class Clock {

public:
    int hour;
    int minute;
    int second;
    
    std::mutex clk_mutex;

    Clock(int h = 0, int m = 0, int s = 0)
    : hour(h), minute(m), second(s)
    {}

    void jump_in_time(int sec);

    int now();

    std::string print_time();
};