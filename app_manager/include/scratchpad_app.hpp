#ifndef SCRATCHPAD_APP_HPP
#define SCRATCHPAD_APP_HPP

#include "app.hpp"

class ScratchpadApp : public App {
public:
    std::string getName() const override { return "Init Scratchpad Functions"; }
    void run(TIGui& gui) override;
};

#endif // SCRATCHPAD_APP_HPP
