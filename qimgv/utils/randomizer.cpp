#include "randomizer.h"

Randomizer::Randomizer() : currentIndex(0), rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    setCount(0);
}

Randomizer::Randomizer(size_t _count) : currentIndex(0), rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    setCount(_count);
}

void Randomizer::setCount(size_t _count) {
    vec.resize(_count);
    indexMap.resize(_count);
    fill();
}

void Randomizer::shuffle() {
    std::shuffle(vec.begin(), vec.end(), rng);
    for(size_t i = 0; i < vec.size(); ++i)
        indexMap[vec[i]] = i;
}

void Randomizer::setCurrent(size_t _current) {
    currentIndex = indexOf(_current);
}

size_t Randomizer::indexOf(size_t item) {
    if(item >= indexMap.size())
        return size_t(-1);
    return indexMap[item];
}

void Randomizer::fill() {
    for(size_t i = 0; i < vec.size(); ++i) {
        vec[i] = i;
        indexMap[i] = i;
    }
}

size_t Randomizer::next() {
    if(vec.empty()) return 0;
    // re-shuffle when needed
    // because vector gets rearranged this will break prev()
    if(currentIndex >= vec.size() - 1) {
        size_t currentItem = vec[currentIndex];
        shuffle();
        setCurrent(currentItem);
    }
    currentIndex++;
    return vec[currentIndex];
}

size_t Randomizer::prev() {
    if(vec.empty()) return 0;
    if(currentIndex == 0) {
        size_t currentItem = vec[currentIndex];
        shuffle();
        setCurrent(currentItem);
    }
    currentIndex--;
    return vec[currentIndex];
}
