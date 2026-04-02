#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include <QDebug>
#include <QString>

class Randomizer {
public:
    Randomizer();
    Randomizer(size_t _count);

    void setCount(size_t _count);
    size_t next();
    size_t prev();

    void shuffle();
    void print();
    void setCurrent(size_t _current);
private:
    size_t currentIndex;
    std::vector<int> vec;
    std::vector<int> indexMap;  // indexMap[value] = position in vec
    std::mt19937 rng;  // 随机数生成器作为成员变量，避免重复创建
    void fill();
    size_t indexOf(size_t item);
};
