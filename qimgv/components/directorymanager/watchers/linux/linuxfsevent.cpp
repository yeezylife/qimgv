#include "linuxfsevent.h"

LinuxFsEvent::LinuxFsEvent(char *data, uint dataSize) :
    mData(data),
    mDataSize(dataSize)
{
}

LinuxFsEvent::~LinuxFsEvent() {
}

uint LinuxFsEvent::dataSize() const {
    return 0;
}

void LinuxFsEvent::setDataSize(uint bufferSize) {
    Q_UNUSED(bufferSize);
}

char *LinuxFsEvent::data() const {
    return nullptr;
}

void LinuxFsEvent::setData(char *buffer) {
    Q_UNUSED(buffer);
}