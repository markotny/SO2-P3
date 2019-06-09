
class Resource {
public:
    std::string name;
    int capacity, used_by;
    int x,y;

    std::mutex resmutex;
    std::condition_variable rescondition;

    Resource(std::string n, int iks, int igrek, int cap = 1)
    : name(n), x(iks), y(igrek), capacity(cap) {
        used_by = 0;
    }
};