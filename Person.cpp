#include "Person.h"
#include <cmath>
#include <ncurses.h>
#include <thread>
#include <iostream>
#include <algorithm>
#include "easylogging++.h"


void Person::sleep(){
    move_to_resource_used(1);
    state = sleeping;
    print_state("sleeping");
    int wake_up = 60 * (360 + std::rand() % 120); // wake up between 6 and 8
    while (main_clock->hour >= 22 || main_clock->now() < wake_up){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    state = idle;
    resource_used = nullptr;
    used_kitchen = false;
}

void move_line(Resource * res, std::deque<Person*> * queue){
    for (int i = 0; i < queue->size(); i++){
        Person * p = queue->at(i);
        std::scoped_lock lk(p->permutex);
        if (p->state == moving) {
            p->dest_x = res->x - 2 * (i + 1);
        } else {
            p->reprint(p->x, p->y, res->x - 2 * (i + 1), p->y);
            p->x = res->x - 2 * (i + 1);
        }
    }
}

void wait_until_avaible(Resource * res){
    res->lock = true;
    while (res->used_by > 0){
        std::unique_lock lk(res->resmutex);
        res->rescondition.wait(lk, [res] {
            return res->used_by == 0;
        });
        lk.unlock();
    }
}

void Person::use(Resource* res, int duration_minutes, std::deque<Person*> * queue){
    queue->push_back(this);
    resource_used = res;
    state = moving;
    print_state("moving to " + res->name);

    move_to_resource_used(queue->size());

    state = waiting;

    std::thread wait_for_req;
    bool locked = false;
    if (res->res_required != nullptr && res->res_required->lock == false){
        wait_for_req = std::thread(wait_until_avaible, res->res_required);
        locked = true;
    }

    print_state ("waiting for " + res->name);
    
    while (res->used_by >= res->capacity || res->lock == true){
        std::unique_lock lk(res->resmutex);
        res->rescondition.wait(lk, [res] {
            return res->used_by < res->capacity && res->lock == false;
        });
        lk.unlock();
    }

    if (res->res_required != nullptr){
        Resource * req = res->res_required;
        print_state ("can use " + res->name + ", waiting for " + req->name);
        if (locked == true)
            if (wait_for_req.joinable()) wait_for_req.join();

        else if (req->used_by > 0) {
            std::unique_lock lk(req->resmutex);
            req->rescondition.wait(lk, [req] {
                return req->used_by == 0;
            });
            lk.unlock();
        }
    }

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

        int elapsed = main_clock->now() - start;
        if (elapsed < 0)
            elapsed = 86400 - elapsed;  // 86400 = 24 * 60 * 60 -> whole day

        progress = static_cast<float>(elapsed) / (duration_minutes * 60);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (progress < 1.0);
    {
        std::scoped_lock lk(res->resmutex);
        state = idle;
        resource_used = nullptr;
        int i = 0;
        queue->erase(std::find(queue->begin(), queue->end(), this));
        move_line(res, queue);
        res->used_by--;
    }
    if (locked == true){
        {
            std::scoped_lock lk(res->res_required->resmutex);
            res->res_required->lock = false;
        }
        res->res_required->rescondition.notify_all();
    }
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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

void Person::print_state(std::string state){
    std::scoped_lock lk (*print_mutex);

    mvprintw(state_line, 80, "%s: %s", name.c_str(), state.c_str());
    clrtoeol();

    refresh();
}