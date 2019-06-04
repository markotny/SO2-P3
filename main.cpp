#include <ncurses.h>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "easylogging++.h"
#include "Resource.h"
#include "Clock.h"
#include "Person.h"

INITIALIZE_EASYLOGGINGPP

int terminal_width;
int terminal_height;

std::mutex print_mutex;
bool cancel = false;

Resource* beds[5], *kitchen, *bathroom, *computer, *tv, *console;
Person* persons[5];
Resource* resources[5];
std::vector<Person*> *queue[5];   // queue for each resource
Person* children[3];

Clock* clock;

void init() {
    initscr();    // init ncurses on whole console
    curs_set(0);  // hide cursor
    timeout(-1);  // blocking getch()

    getmaxyx(stdscr, terminal_height, terminal_width);
    LOG(INFO) << "Started on terminal width x height: " << terminal_width
              << " x " << terminal_height;

    clear();
    endwin();
}

void time_flows(int interval_ms){
    while(!cancel){
        clock->jump_in_time(60);
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
        if (person->state != idle){
            std::unique_lock lk(person->permutex);
            person->percondition.wait(lk, [person] {
                return person->state == idle || cancel == true;
            });
            lk.unlock();
        }
        if (clock->hour >= 22 || clock->hour < 6){
            for (int i = 0; i < 5; i++){
                if (!beds[i]->used){
                    person->resource_used = beds[i];
                    break;
                }
            }
            person->sleep();
        } else if (clock->hour >= 13 && clock->hour <= 18 && !person->used_kitchen){
            queue[0]->push_back(person);
            person->use(kitchen, std::rand() % 60, queue[0]);
        } else {
            int res = std::rand() % 4 + 1;
            queue[res]->push_back(person);
            person->use(resources[res], std::rand() % 60, queue[res]);
        }
    }
}


void house_setup(){
    persons[0] = new Person("mama", clock);
    persons[1] = new Person("tata", clock);
    persons[2] = new Person("Igor", clock);
    persons[3] = new Person("Irek", clock);
    persons[4] = new Person("Iga", clock);

    for (int i = 0; i < 5; i++){
        beds[i] = new Resource(4, 1 + 2 * i);
    }

    kitchen = new Resource(40, 1, 2);
    bathroom = new Resource(40, 3);
    computer = new Resource(40, 5);
    tv = new Resource(40, 7, 3);
    console = new Resource(40, 9, 2);

    resources[0] = kitchen;
    resources[1] = bathroom;
    resources[2] = computer;
    resources[3] = tv;
    resources[4] = console;

    children[0] = persons[2];
    children[1] = persons[3];
    children[2] = persons[4];

    clock = new Clock(8, 0, 0);
}

void delete_house(){
    delete[] persons;
    delete[] beds;
    delete[] resources;
    delete[] queue;
    delete clock;
}
int main() {
    std::srand(time(NULL));
    el::Configurations conf("log/easylogging.conf");
    conf.set(el::Level::Global, el::ConfigurationType::Enabled, "true");
    
    el::Loggers::reconfigureLogger("default", conf);

    //init();
    house_setup();

    std::thread clock_thread = std::thread(time_flows, 100);
    std::thread ppl_threads[5];


    for (int i = 0; i < 5; i++){
        ppl_threads[i] = std::thread(live_a_life, persons[i]);
    }

    std::thread(stop_all).join();

    clock_thread.join();
    for (int i = 0; i < 5; i++){
        if (ppl_threads[i].joinable())
            ppl_threads[i].join();
    }

    delete_house();
    return 0;
}