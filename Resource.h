
class Resource {
public:
    std::string name;
    bool used;
    int capacity;
    int x,y;
    Resource(std::string n, int iks, int igrek, int cap = 1)
    : name(n), x(iks), y(igrek), capacity(cap) {
        used = false;
    }
};