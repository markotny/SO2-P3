#include "Person.h"
#include <cmath>
#include <thread>
#include <iostream>


void Person::sleep(){
    move_to_resource_used(1);
    //wait on conditional variable...
}

void Person::use(Resource* res, int duration_minutes, std::vector<Person*> * queue){
    resource_used = res;
    state = moving;
    std::cout<< name << " starting to move\n";

    //move_to_resource_used(queue->size());

    state = waiting;
    int position_in_queue = queue->size() - 1;

    std::cout<< name << " waiting in queue at " << position_in_queue << std::endl;

    while (position_in_queue >= res->capacity){
        Person * pers = queue->at(position_in_queue - 1); // lock on previous guy in line

        std::unique_lock lk(pers->permutex);
        pers->percondition.wait(lk, [pers] {
            return pers->state == idle;
        });
        position_in_queue--;
        //x += 2; // move in line
        std::cout<< name << " moved in line\n";
        lk.unlock();
    }

    std::cout<< name << " starting to use resource\n";
    state = using_resource;
    int start = get_time();
    while (get_time() - start < duration_minutes * 60){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout<< name << " stopped using resource\n";
    {
        std::scoped_lock lk(permutex);
        state = idle;
        resource_used = nullptr;
        queue->erase(queue->begin() + position_in_queue);
    }
    percondition.notify_all();
}

void Person::move_to_resource_used(int queue_size){
    start_x = x;
    start_y = y;
    dest_x = resource_used->x - 2 * queue_size;
    dest_y = resource_used->y;
    vec_x = dest_x - x;
    vec_y = dest_y - y;
    steps_left = sqrt(pow(vec_x, 2) + pow(vec_y, 2));
    steps_all = steps_left;
    keep_on_movin();
}

void Person::keep_on_movin(){
    while (steps_left > 0){
        x = start_x + vec_x * (steps_all - steps_left);
        y = start_y + vec_y * (steps_all - steps_left);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    x = dest_x;
    y = dest_y;
}

int Person::get_time() {
    std::scoped_lock lk(main_clock->clk_mutex);
    return main_clock->now();
}
