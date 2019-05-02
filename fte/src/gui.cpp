
#include "gui.h"

int GFrame::isLastFrame() {
    return (this == Next && this == frames);
}

int GUI::Start(int &/*argc*/, char ** /*argv*/) {
    return 1;
}

void GUI::Stop() {
}

void GUI::StopLoop() {
    doLoop = 0;
}
