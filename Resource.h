
class Resource {
public:
    std::string name;
    int capacity, used_by, min_required;
    int x,y;
    bool lock;  // if required by other res
    Resource * res_required;

    std::mutex resmutex;
    std::condition_variable rescondition;

    Resource(std::string n, int iks, int igrek, int cap = 1, int min = 1, Resource * res = nullptr)
    : name(n), x(iks), y(igrek), capacity(cap), min_required(min), res_required(res) {
        used_by = 0;
        lock = false;
    }
};