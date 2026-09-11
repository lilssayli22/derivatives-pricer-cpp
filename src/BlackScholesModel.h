#pragma once


#include <pnl/pnl_matrix.h>
#include <pnl/pnl_random.h>
#include <pnl/pnl_vector.h>

class BlackScholesModel {
public:
    int D;
    double r;
    double rho;
    PnlVect* sigma;
    PnlVect* S0;
    PnlMat* L;

    // ajout constructeur qui perm de calcul la matrice L : 
     BlackScholesModel(int D, double r, double rho, PnlVect* sigma, PnlVect* S0);
    void asset( PnlMat* past, double t, double T, int N, PnlMat* path, PnlRng* rng);
    void asset(double T, int N, PnlMat* simulation, PnlRng* rng);
    PnlVect* shift_asset(PnlVect* spot, int d, double h, int sign) ;

 static int compute_last_index(double t, double T, int N) ;
    void shift_asset(PnlMat* shifted, PnlMat* path, int d, double h,
                      double t, double T, int N);

   PnlMat* generate_gaussianMat(int N, PnlRng* rng);

   void simule_etape(const PnlMat* source,int source_index,PnlMat* simulation,const PnlMat* gaussian,int gaussian_index,int destination_index,double pas);


   
    
};
