#include <ncurses.h>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "Person.h"
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP
#define ELPP_THREAD_SAFE

int terminal_width;
int terminal_height;

std::mutex print_mutex;
bool cancel = false;

Resource* beds[5], *kitchen, *bathroom, *computer, *tv, *console;
Person* persons[5];
Resource* resources[5];
std::deque<Person*> *queue[5];   // queue for each resource
Person* children[3];

Clock* main_clock;

void init_scr() {
    initscr();    // init_scr ncurses on whole console
    curs_set(0);  // hide cursor
    timeout(-1);  // blocking getch()

    getmaxyx(stdscr, terminal_height, terminal_width);
    LOG(INFO) << "Started on terminal width x height: " << terminal_width
              << " x " << terminal_height;
}

void time_flows(int interval_ms){
    while(!cancel){
        std::scoped_lock lk(main_clock->clk_mutex);
        main_clock->jump_in_time(60);
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

    LOG(INFO) << "stopping...";
    cancel = true;
}

void live_a_life(Person* person){
    while(!cancel){
        if (main_clock->hour >= 22 || main_clock->hour < 6){
            for (int i = 0; i < 5; i++){
                if (beds[i]->used_by == 0){
                    person->resource_used = beds[i];
                    beds[i]->used_by++;
                    LOG(INFO) << person->name << "going to bed " << i << std::endl;
                    break;
                }
            }
            person->sleep();
            LOG(INFO) << main_clock->print_time() << person->name << " woke up" << std::endl;
            

        } else if (main_clock->hour >= 13 && main_clock->hour <= 18 && !person->used_kitchen){
            queue[0]->push_back(person);
            LOG(INFO) << main_clock->print_time() << person->name << " going to kitchen" << std::endl;

            person->use(kitchen, 60 + std::rand() % 60, queue[0]);
            person->used_kitchen = true;
            LOG(INFO) << main_clock->print_time() << person->name << " done with kitchen" << std::endl;
        } else {
            int res = std::rand() % 4 + 1;
            queue[res]->push_back(person);
            LOG(INFO) << main_clock->print_time() << person->name << " going for resource " << res << std::endl;
            person->use(resources[res], 60 + std::rand() % 120, queue[res]);
            LOG(INFO) << main_clock->print_time() << person->name << " done with resource " << res << std::endl;
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

    kitchen = new Resource("kitchen", 50, 1, 2);
    bathroom = new Resource("bathroom", 20, 3);
    computer = new Resource("computer", 60, 7);
    tv = new Resource("tv", 30, 10, 3);
    console = new Resource("console", 30, 12, 2);

    resources[0] = kitchen;
    resources[1] = bathroom;
    resources[2] = computer;
    resources[3] = tv;
    resources[4] = console;

    children[0] = persons[2];
    children[1] = persons[3];
    children[2] = persons[4];

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
    el::Configurations conf("log/easylogging.conf");
    conf.set(el::Level::Global, el::ConfigurationType::Enabled, "false");
    
    el::Loggers::reconfigureLogger("default", conf);

    init_scr();
    house_setup();

    std::thread print_house_thread = std::thread(print_house);
    std::thread clock_thread = std::thread(time_flows, 50);
    std::thread ppl_threads[5];

    for (int i = 0; i < 5; i++){
        ppl_threads[i] = std::thread(live_a_life, persons[i]);
    }
    LOG(INFO) << "Started all threads.\n";

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