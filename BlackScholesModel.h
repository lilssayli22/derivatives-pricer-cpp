# include <vector>
class BlackScholesModel {
public:
    int D;
    double r;
    double rho;
    std::vector<double> sigma;
    std::vector<double> S0;
    std::vector<std::vector<double>> L;

    std::vector<std::vector<double>> asset(double T, int N);
};
