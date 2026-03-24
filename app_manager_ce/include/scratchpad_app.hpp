#ifndef SCRATCHPAD_APP_HPP
#define SCRATCHPAD_APP_HPP

#include "app.hpp"

class ScratchpadApp : public App {
public:
    const char* getName() const override { return "Python Formula Ref"; }
    void run(TIGui& gui) override;
};

#endif // SCRATCHPAD_APP_HPP
