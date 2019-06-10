#include <ncurses.h>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "Person.h"

int terminal_width;
int terminal_height;

std::mutex print_mutex;
bool cancel = false;

Resource* beds[5];
Person* persons[5];
Resource* resources[5];
std::deque<Person*> *queue[5];   // queue for each resource

Clock* main_clock;

void init_scr() {
    initscr();    // init_scr ncurses on whole console
    curs_set(0);  // hide cursor
    timeout(-1);  // blocking getch()

    getmaxyx(stdscr, terminal_height, terminal_width);
}

void the_time_is_now(int interval_ms){
    while(!cancel){
        std::scoped_lock lk(main_clock->clk_mutex);
        if (main_clock->hour > 6)
            main_clock->jump_in_time(60);
        else
            main_clock->jump_in_time(180);
        {
            std::scoped_lock loc(print_mutex);
            mvprintw(1, 80, "Time: %s", main_clock->print_time().c_str());
            refresh();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

void stop_all() {
    getch();
    cancel = true;
}

void live_a_life(Person* person){
    while(!cancel){
        int prev_res = 0;
        if (main_clock->hour >= 22 || main_clock->hour < 6){
            for (int i = 0; i < 5; i++){
                if (beds[i]->used_by == 0){
                    person->resource_used = beds[i];
                    break;
                }
            }
            person->sleep();

        } else if (main_clock->hour >= 13 && main_clock->hour <= 18 && !person->used_kitchen){

            person->use(resources[0], 60 + std::rand() % 60, queue[0]);
            person->used_kitchen = true;
        } else {
            int res;
            do {
                res = std::rand() % 4 + 1; // rand without kitchen
            } while (res == prev_res);
            prev_res = res;
            
            person->use(resources[res], 60 + std::rand() % 120, queue[res]);
        }
    }
}


void house_setup(){
    main_clock = new Clock(8, 0, 0);

    persons[0] = new Person("mama", 3, main_clock, &print_mutex);
    persons[1] = new Person("tata", 4, main_clock, &print_mutex);
    persons[2] = new Person("Janusz", 5, main_clock, &print_mutex);
    persons[3] = new Person("Marcin", 6, main_clock, &print_mutex);
    persons[4] = new Person("Kuba", 7, main_clock, &print_mutex);

    for (int i = 0; i < 5; i++){
        beds[i] = new Resource("bed", 4, 1 + 2 * i);
        queue[i] = new std::deque<Person*>();
    }

    resources[0] = new Resource("kitchen", 50, 1, 2);
    resources[1] = new Resource("bathroom", 20, 3);
    resources[2] = new Resource("computer", 60, 7);
    resources[3] = new Resource("tv", 30, 10, 3);
    resources[4] = new Resource("console", 30, 12, 2, 2, resources[3]);
}

void print_house(){
    while (!cancel) {
        {
            std::scoped_lock lk(print_mutex);
            for (int i = 0; i < 5; i++) {
                mvprintw(beds[i]->y, beds[i]->x, "bed%d", i);
                mvprintw(resources[i]->y, resources[i]->x, "%s",
                         resources[i]->name.c_str());
            }
            refresh();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void delete_house(){
    for (int i = 0; i < 5; i++){
        delete persons[i];
        delete beds[i];
        delete resources[i];
        delete queue[i];
    }
    delete main_clock;
}

int main() {
    std::srand(time(NULL));

    init_scr();
    house_setup();

    std::thread print_house_thread = std::thread(print_house);
    std::thread clock_thread = std::thread(the_time_is_now, 50);
    std::thread ppl_threads[5];

    for (int i = 0; i < 5; i++){
        ppl_threads[i] = std::thread(live_a_life, persons[i]);
    }

    std::thread(stop_all).join();

    print_house_thread.join();
    clock_thread.join();
    for (int i = 0; i < 5; i++){
        if (ppl_threads[i].joinable())
            ppl_threads[i].join();
    }

    delete_house();

    clear();
    endwin();
    return 0;
}