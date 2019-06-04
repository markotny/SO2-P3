
class Resource {
public:
    bool used;
    int capacity;
    int x,y;
    Resource(int iks, int igrek, int cap = 1)
    : x(iks), y(igrek), capacity(cap) {
        used = false;
    }
};