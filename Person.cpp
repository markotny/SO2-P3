#include "Person.h"
#include <cmath>
#include <ncurses.h>
#include <thread>
#include <iostream>
#include "easylogging++.h"


void Person::sleep(){
    move_to_resource_used(1);
    //wait until the morning...
}

void Person::use(Resource* res, int duration_minutes, std::deque<Person*> * queue){
    resource_used = res;
    state = moving;
    LOG(INFO)<< name << " starting to move\n";

    move_to_resource_used(queue->size());

    state = waiting;
    int position_in_queue = queue->size() - 1;

    LOG(INFO)<< name << " waiting in queue at " << position_in_queue << std::endl;

    while (position_in_queue >= res->capacity){
        Person * pers = queue->at(position_in_queue - 1); // lock on previous guy in line

        std::unique_lock lk(pers->permutex);
        pers->percondition.wait(lk, [pers] {
            return pers->state == idle;
        });
        position_in_queue--;
        reprint(x, y, x + 2, y);
        x += 2;
        LOG(INFO)<< name << " moved in line\n";
        lk.unlock();
    }

    LOG(INFO)<< name << " starting to use resource\n";
    state = using_resource;
    {
        std::scoped_lock lk (*print_mutex);
        mvaddch(y,x, name.c_str()[0] | A_UNDERLINE);
        refresh();
    }
    int start = main_clock->now();
    while (main_clock->now() - start < duration_minutes * 60){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    {
        std::scoped_lock lk(permutex);
        state = idle;
        resource_used = nullptr;
        queue->erase(queue->begin() + position_in_queue);

        reprint(x, y, x, y - 1);
        y -= 1;
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
        int xn = start_x + vec_x * static_cast<float>(steps_all - steps_left) / steps_all;
        int yn = start_y + vec_y * static_cast<float>(steps_all - steps_left) / steps_all;
        reprint(x, y, xn, yn);
        x = xn;
        y = yn;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        steps_left--;
    }
    reprint(x, y, dest_x, dest_y);
    x = dest_x;
    y = dest_y;
}

void Person::reprint(int x_old, int y_old, int x_new, int y_new){
    std::scoped_lock lk (*print_mutex);

    mvprintw(y_old, x_old, " ");
    mvprintw(y_new, x_new, "%c", name.c_str()[0]);

    refresh();
}