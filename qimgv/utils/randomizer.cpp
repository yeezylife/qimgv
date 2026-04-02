#include "randomizer.h"

Randomizer::Randomizer() {
    setCount(0);
}

Randomizer::Randomizer(size_t _count) : currentIndex(0) {
    setCount(_count);
}

void Randomizer::setCount(size_t _count) {
    vec.resize(_count);
    indexMap.resize(_count);
    fill();
}

void Randomizer::shuffle() {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::shuffle(vec.begin(), vec.end(), rng);
    for(size_t i = 0; i < vec.size(); ++i)
        indexMap[vec[i]] = static_cast<int>(i);
}

void Randomizer::setCurrent(size_t _current) {
    currentIndex = indexOf(_current);
}

size_t Randomizer::indexOf(size_t item) {
    if(item >= indexMap.size())
        return size_t(-1);
    return static_cast<size_t>(indexMap[item]);
}

void Randomizer::fill() {
    for(size_t i = 0; i < vec.size(); ++i) {
        vec[i] = static_cast<int>(i);
        indexMap[i] = static_cast<int>(i);
    }
}

void Randomizer::print() {
    qDebug() << "---vector---";
    for(int v : vec)
        std::cout << v << '\n';
    qDebug() << "----end----";
}

size_t Randomizer::next() {
    // re-shuffle when needed
    // because vector gets rearranged this will break prev()
    while(currentIndex == vec.size() - 1) {
        size_t currentItem = vec[currentIndex];
        shuffle();
        setCurrent(currentItem);
    }
    currentIndex++;
    return vec[currentIndex];
}

size_t Randomizer::prev() {
    while(currentIndex == 0) {
        size_t currentItem = vec[currentIndex];
        shuffle();
        setCurrent(currentItem);
    }
    currentIndex--;
    return vec[currentIndex];
}
