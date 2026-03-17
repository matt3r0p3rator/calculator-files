#ifndef GAUSS_APP_HPP
#define GAUSS_APP_HPP

#include "app.hpp"

class GaussApp : public App {
public:
    std::string getName() const override { return "Gauss-Jordan Elimination"; }
    void run(TIGui& gui) override;
};

#endif // GAUSS_APP_HPP
