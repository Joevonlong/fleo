#include "controller.h"

Define_Module(Controller);

int Controller::numInitStages() const {
    return 1;
}

void Controller::initialize(int stage) {

}

void Controller::finish() {
}

Flow* Controller::createFlow(Path path, uint64_t bps, Priority p) {

}
Flow* Controller::createFlow(Flow* f) {

}

bool Controller::revokeFlow(Flow* f) {

}
