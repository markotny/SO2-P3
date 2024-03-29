#include "Clock.h"
#include <iostream>

void Clock::jump_in_time(int sec){
    second += sec;
    if (second >= 60){
        minute += second / 60;
        second = second % 60;
    }
    if (minute >= 60){
        hour += minute / 60;
        minute = minute % 60;
    }
    if (hour >= 24){
        hour = hour % 24;
    }
}

int Clock::now(){
    return second + minute * 60 + hour * 3600;
}

std::string Clock::print_time(){
    return std::string(std::to_string(hour) + ":" + std::to_string(minute) + ":" + std::to_string(second) + " ");
}