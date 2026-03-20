#ifndef INVERSE_APP_HPP
#define INVERSE_APP_HPP

#include "app.hpp"
#include "inverse_solver.hpp"
#include "ti_gui.hpp"

class InverseApp : public App {
public:
    std::string getName() const override { return "Matrix Inverse"; }
    void run(TIGui& gui) override;
};

#endif // INVERSE_APP_HPP