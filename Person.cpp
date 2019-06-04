#include "Person.h"
#include <cmath>
#include <thread>


void Person::sleep(){
    move_to_resource_used(1);
    //wait on conditional variable...
}


void Person::move_to_resource_used(int queue_size){
    state = moving;
    start_x = x;
    start_y = y;
    dest_x = resource_used->x - 2 * queue_size;
    dest_y = resource_used->y;
    vec_x = dest_x - x;
    vec_y = dest_y - y;
    steps_left = sqrt(pow(vec_x, 2) + pow(vec_y, 2));
    steps_all = steps_left;
    std::thread(make_a_move).join();
    if (queue_size > resource_used->capacity)
        state = waiting;
}

void Person::make_a_move(){
    while (steps_left > 0){
        x = start_x + vec_x * (steps_all - steps_left);
        y = start_y + vec_y * (steps_all - steps_left);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    x = dest_x;
    y = dest_y;
}
