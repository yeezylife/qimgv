#pragma once

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

class Randomizer {
public:
    Randomizer();
    Randomizer(size_t _count);

    void setCount(size_t _count);
    size_t next();
    size_t prev();

    void shuffle();
    void setCurrent(size_t _current);
private:
    size_t currentIndex;
    std::vector<size_t> vec;
    std::vector<size_t> indexMap;  // indexMap[value] = position in vec
    std::mt19937 rng;
    void fill();
    size_t indexOf(size_t item);
};
