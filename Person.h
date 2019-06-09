#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include "Resource.h"
#include "Clock.h"

enum pstate {
    waiting,
    using_resource,
    idle,
    moving
};

class Person {
public:
    std::string name;
    pstate state;
    Resource* resource_used;
    bool used_kitchen;
    int x,y;

    Clock* main_clock;
    std::mutex permutex;
    std::condition_variable percondition;
    std::mutex * print_mutex;
    
    Person(std::string n, Clock* cl, std::mutex * pm){
        main_clock = cl;
        name = n;
        print_mutex = pm;
        state = idle;
        used_kitchen = false;
        x = 20;
        y = 5;
    }

    //void move(dest_x, dest_y);
    void sleep();
    void use(Resource* res, int minutes, std::deque<Person*> * queue);

private:
    int steps_all, steps_left, start_x, start_y, dest_x, dest_y, vec_x, vec_y;
    void move_to_resource_used(int queue_size);
    void keep_on_movin();
    void reprint(int x_old, int y_old, int x_new, int y_new);
};