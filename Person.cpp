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
    print_state("moving to " + res->name);

    move_to_resource_used(queue->size());

    state = waiting;

    while (res->used_by >= res->capacity){
        print_state ("waiting for " + res->name);
        // lock on the resource

        std::unique_lock lk(res->resmutex);
        res->rescondition.wait(lk, [res] {
            return res->used_by < res->capacity; //wait until next guy moves
        });
        lk.unlock();
    }

    print_state("using " + res->name + " 0%");
    state = using_resource;
    res->used_by++;
    
    int start = main_clock->now();
    float progress = 0.0;
    do {
        {
            std::scoped_lock lk (*print_mutex);
            mvaddch(y,x, name.c_str()[0] | A_UNDERLINE);
            refresh();
        }
        print_state("using " + res->name + " - " + std::to_string(int(100 * progress)) + "%");
        progress = static_cast<float>(main_clock->now() - start) / (duration_minutes * 60);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (progress < 1.0);
    {
        std::scoped_lock lk(permutex, res->resmutex);
        state = idle;
        resource_used = nullptr;
        int i = 0;
        for (; i < queue->size(); i++){
            if (queue->at(i) == this)
                break;
        }
        queue->erase(queue->begin() + i);
        for (; i < queue->size(); i++){
            queue->at(i)->move_in_line();
        }
        res->used_by--;
        reprint(x, y, x, y - 1);
        y -= 1;
    }
    percondition.notify_all();
    res->rescondition.notify_all();
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
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        steps_left--;
    }
    reprint(x, y, dest_x, dest_y);
    x = dest_x;
    y = dest_y;
}

void Person::move_in_line(){
    reprint(x, y, x + 2, y);
    x += 2;
}

void Person::reprint(int x_old, int y_old, int x_new, int y_new){
    std::scoped_lock lk (*print_mutex);

    mvprintw(y_old, x_old, " ");
    mvprintw(y_new, x_new, "%c", name.c_str()[0]);

    refresh();
}

void Person::print_state(std::string state){
    std::scoped_lock lk (*print_mutex);

    mvprintw(state_line, 80, "%s: %s", name.c_str(), state.c_str());
    clrtoeol();

    refresh();
}