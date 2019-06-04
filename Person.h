#include <string>
#include "Resource.h"

enum action {
    using_resource,
    waiting,
    moving
};

class Person {
public:
    std::string name;
    action state;
    Resource* resource_used;
    bool used_kitchen;
    int x,y;
    
    Person(std::string n){
        name = n;
        state = moving;
        used_kitchen = false;
    }

    //void move(dest_x, dest_y);
    void sleep();
    void use(Resource* res, int queue_size);

private:
    int steps_all, steps_left, start_x, start_y, dest_x, dest_y, vec_x, vec_y;
    void move_to_resource_used(int queue_size);
    void make_a_move();
};