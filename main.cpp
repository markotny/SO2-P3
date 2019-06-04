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

Resource beds[5], kitchen, bathroom, computer, tv, console;
Person persons[5];
Resource* resources[5];
std::vector<Person*> queue[5];   // queue for each resource
Person* children[3];

Clock clock;

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
        clock.jump_in_time(60);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

void stop_all() {
    getch();

    LOG(INFO) << "stopping...";
    //std::scoped_lock lk(left_mutex, right_mutex);
    cancel = true;
    // left_cv.notify_all();
    // right_cv.notify_all();
}

void live_a_life(Person* person){
    while(!cancel){
        //wait on conditional unil person finished doing whatever they were doing
        if (clock.hour >= 22 || clock.hour < 6){
            for (int i = 0; i < 5; i++){
                if (!beds[i].used){
                    person->resource_used = &beds[i];
                    break;
                }
            }
            person->sleep();
        } else if (clock.hour >= 13 && clock.hour <= 18 && !person->used_kitchen){
            queue[0].push_back(person);
            person->use(&kitchen, queue[0].size);
        } else {
            int res = std::rand() % 4 + 1;
            queue[res].push_back(person);
            person->use(resources[res], queue[res].size);
        }
    }
}


void house_setup(){
    persons[0] = Person("mama");
    persons[1] = Person("tata");
    persons[2] = Person("Igor");
    persons[3] = Person("Irek");
    persons[4] = Person("Iga");

    for (int i = 0; i < 5; i++){
        beds[i] = Resource(4, 1 + 2 * i);
    }

    kitchen = Resource(40, 1, 2);
    bathroom = Resource(40, 3);
    computer = Resource(40, 5);
    tv = Resource(40, 7, 3);
    console = Resource(40, 9, 2);

    resources[0] = &kitchen;
    resources[1] = &bathroom;
    resources[2] = &computer;
    resources[3] = &tv;
    resources[4] = &console;

    children[0] = &persons[2];
    children[1] = &persons[3];
    children[2] = &persons[4];

    clock = Clock(8, 0, 0);

    for (int i = 0; i < 5; i++){
        int res = std::rand() % 5;
        persons[i].resource_used = resources[res];
        queue[res].push_back(&persons[i]);

        persons[i].use(resources[res], queue[res].size);
    }
}
int main() {
    std::srand(time(NULL));
    el::Configurations conf("log/easylogging.conf");
    conf.set(el::Level::Global, el::ConfigurationType::Enabled, "true");
    
    el::Loggers::reconfigureLogger("default", conf);

    //init();
    house_setup();

    std::thread clock_thread = std::thread(time_flows, 100);
    std::thread(stop_all).join();

    clock_thread.join();

    return 0;
}