
#include "refdata.h"

using namespace insight;

int main(int argc, char*argv[])
{
    arma::mat refdata_umean180=refdatalib.getProfile("MKM_Channel", "180/umean_vs_yp");
    
    std::cout<<"MKM_umean180 = "<<refdata_umean180<<std::endl;
    if (refdata_umean180.n_rows==0) return -1;
    
    return 0;
}

