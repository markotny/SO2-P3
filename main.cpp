#include <ncurses.h>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "Dot.h"
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

int terminal_width;
int terminal_height;
int left_line;
int right_line;
const int max_dots = 10;
const int intereval = 500;
Dot dots[max_dots];
std::thread dot_threads[max_dots];

std::mutex print_mutex;
bool cancel = false;

std::mutex left_mutex;
std::condition_variable left_cv;
std::mutex right_mutex;
std::condition_variable right_cv;
int in_left = 0;  // number of dots inside left field
int in_right = 0;
const int max_capacity = 2;

void move_dot(Dot* dot) {
    {
        std::scoped_lock lock(print_mutex);

        mvprintw(dot->y, dot->x, " ");
        dot->move();
        mvprintw(dot->y, dot->x, "o");

        refresh();
    }

    switch (dot->state) {
        case moving_center:
            if (dot->x <= left_line && dot->x_dir < 0) {
                if (in_left < max_capacity) {
                    in_left++;
                    dot->state = moving_left;
                    LOG(INFO) << "Dot " << dot->id << " entering left no wait";
                } else {
                    LOG(INFO) << "Dot " << dot->id << " waiting for left";

                    std::unique_lock lk(left_mutex);
                    left_cv.wait(lk, [] {
                        return in_left < max_capacity || cancel == true;
                    });

                    in_left++;
                    dot->state = moving_left;
                    LOG(INFO) << "Dot " << dot->id << " entering left";

                    lk.unlock();
                }
            } else if (dot->x >= right_line && dot->x_dir > 0) {
                if (in_right < max_capacity) {
                    in_right++;
                    dot->state = moving_right;
                    LOG(INFO) << "Dot " << dot->id << " entering right no wait";
                } else {
                    LOG(INFO) << "Dot " << dot->id << " waiting for right";

                    std::unique_lock lk(right_mutex);
                    right_cv.wait(lk, [] {
                        return in_right < max_capacity || cancel == true;
                    });

                    in_right++;
                    dot->state = moving_right;
                    LOG(INFO) << "Dot " << dot->id << " entering right";

                    lk.unlock();
                }
            }
            break;

        case moving_left:
            if (dot->x >= left_line && dot->x_dir > 0) {
                {
                    std::scoped_lock lk(left_mutex);
                    in_left--;
                    dot->state = moving_center;
                    LOG(INFO) << "Dot " << dot->id << " leaving left";
                }
                left_cv.notify_one();
            }
            break;

        case moving_right:
            if (dot->x <= right_line && dot->x_dir < 0) {
                {
                    std::scoped_lock lk(right_mutex);
                    in_right--;
                    dot->state = moving_center;
                    LOG(INFO) << "Dot " << dot->id << " leaving right";
                }
                right_cv.notify_one();
            }
            break;

        default:
            break;
    }
}

void clear_dot(Dot* dot) {
    std::scoped_lock lock(print_mutex);

    mvprintw(dot->y, dot->x, " ");
    refresh();
}

void dot_thread(Dot* dot) {
    LOG(INFO) << "Dot " << dot->id << " started moving";
    int gravity_iter = 0;
    while (dot->velocity >= 10 && cancel == false) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds((int)std::round(1000 / dot->velocity)));

        move_dot(dot);
        dot->velocity -= 0.25;
        if (gravity_iter < 25)
            gravity_iter++;
        else {
            gravity_iter = 0;
            dot->y_dir += 1;
        }
    }
    clear_dot(dot);

    if (dot->state == moving_left) {
        {
            std::scoped_lock lk(left_mutex);
            in_left--;
            LOG(INFO) << "Dot " << dot->id << " stopped in left";
        }
        left_cv.notify_one();

    } else if (dot->state == moving_right) {
        {
            std::scoped_lock lk(right_mutex);
            in_right--;
            LOG(INFO) << "Dot " << dot->id << " stopped in right";
        }
        right_cv.notify_one();
    }

    if (!cancel) {
        dot->reset();
        LOG(INFO) << "Dot " << dot->id << " reset";
    } else {
        dot->state = stopped;
        LOG(INFO) << "Dot " << dot->id << " stopped";
    }
}

void shoot_dots(Dot dots[]) {
    LOG(INFO) << "Started shoot_dots thread";
    while (!cancel) {
        for (size_t i = 0; i < max_dots; i++) {
            if (cancel)
                dots[i].state = stopped;
            else if (dots[i].state == waiting) {
                if (dot_threads[i].joinable())
                    dot_threads[i].join();
                
                dots[i].state = moving_center;
                dot_threads[i] = std::thread(dot_thread, &dots[i]);

                std::this_thread::sleep_for(std::chrono::milliseconds(intereval));
            }
        }
    }
}

void stop_all() {
    getch();

    LOG(INFO) << "stopping...";
    std::scoped_lock lk(left_mutex, right_mutex);
    cancel = true;
    left_cv.notify_all();
    right_cv.notify_all();
}

void init() {
    initscr();    // init ncurses on whole console
    curs_set(0);  // hide cursor
    timeout(-1);  // blocking getch()

    getmaxyx(stdscr, terminal_height, terminal_width);
    LOG(INFO) << "Started on terminal width x height: " << terminal_width
              << " x " << terminal_height;

    left_line = terminal_width / 3;
    right_line = 2 * terminal_width / 3;

    for (size_t i = 0; i < max_dots; i++) 
        dots[i] = Dot(i, terminal_width, terminal_height);

    std::thread shoot_dots_thread = std::thread(shoot_dots, dots);
    std::thread(stop_all).join();

    shoot_dots_thread.join();
    for (size_t i = 0; i <= max_dots; i++) {
        if (dot_threads[i].joinable()) 
            dot_threads[i].join();
    }

    clear();
    endwin();
}

int main() {
    std::srand(time(NULL));
    el::Configurations conf("log/easylogging.conf");
    conf.set(el::Level::Global, el::ConfigurationType::Enabled, "false");
    el::Loggers::reconfigureLogger("default", conf);

    init();
    return 0;
}