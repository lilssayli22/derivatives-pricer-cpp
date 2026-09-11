//
// Created by ILYASS on 04/09/2026.
//

#include "Option.h"
#include <string>
#include <algorithm>

double Option::payoff(const PnlMat* path) {
    if (optionType == "basket"){
        double basketValue = 0.0;

      for (int d = 0; d < D; d++) {
          basketValue += GET(lambda, d) * MGET(path, N, d);
      }

      return std::max(0.0, basketValue - Strike);
    }
    if (optionType == "asian"){
        PnlVect* I = pnl_mat_mult_vect(path, this->lambda); //recup un vec des sommes sur D à chaque date
        double mean = pnl_vect_sum(I) / (this->N + 1);
        pnl_vect_free(&I);
        return std::max(0.0, mean - this->Strike);
    }
    if (optionType == "performance"){
        PnlVect* I = pnl_mat_mult_vect(path, this->lambda);
        double payoff = 1.0;
        for (int i = 1; i <= this->N; i++){
        payoff += std::max(0.0, GET(I, i) / GET(I, i - 1) - 1.0);}
        pnl_vect_free(&I);
        return payoff;
    }
    
    return 0.0;}
    
        
        
        
        
        
        

  
