#include "BlackScholesModel.h"
#include "Option.h"

class MonteCarlo {
public:
    BlackScholesModel model;
    Option option;
    int M;

    double price();
};
