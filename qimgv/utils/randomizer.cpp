#include "randomizer.h"

Randomizer::Randomizer() {
    setCount(0);
}

Randomizer::Randomizer(size_t _count) : currentIndex(0) {
    setCount(_count);
}

void Randomizer::setCount(size_t _count) {
    vec.resize(_count);
    fill();
}

void Randomizer::shuffle() {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::shuffle(vec.begin(), vec.end(), rng);
}

void Randomizer::setCurrent(size_t _current) {
    currentIndex = indexOf(_current);
}

// this assumes our array contains shuffled int values [0 ... count]
// which it does in our case
size_t Randomizer::indexOf(size_t item) {
    size_t index = size_t(-1);
    if(item >= vec.size())
        return index;
    std::vector<int>::iterator it;
    size_t i = 0;
    for (it = vec.begin(); it != vec.end(); ++it, ++i) {
        if(vec[i] == item) {
            index = i;
            break;
        }
    }
    return index;
}

void Randomizer::fill() {
    std::vector<int>::iterator it;
    int i = 0;
    for (it = vec.begin(); it != vec.end(); ++it, ++i) {
        *it = i;
    }
}

void Randomizer::print() {
    qDebug() << "---vector---";
    std::vector<int>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << std::endl;
    }
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
