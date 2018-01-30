#include <stdio.h>
#include "pnl/pnl_vector.h"
#include <pnl/pnl_finance.h>
#include <pnl/pnl_cdf.h>
#include <pnl/pnl_complex.h>

extern "C" {
typedef struct option {
    double strike;
    double call_price;
    double put_price;
	double ivol;
} option;

//compute volatility swap in non-parametric Levy model, fn - file name with option prices
//option strikes in file must be sorted
//s - asset price
//T - time to maturity
double levy_vol_product(const char* fn, double s, double T, double r)
{
    const int max_options=50;
    option options[max_options];
    
	

    //Reading option data from file fn
    
	FILE* f=fopen(fn, "r");
    if (f == NULL)
    {
        perror("Could not open file");
        return 0;
    }
	int n;
	fscanf(f, "%d \n", &n);
	n=0;
    int iatm=0; // index of atm put
    
	////find k0 at-the-money strike price ////////////////
    while (!feof(f))
    {
        fscanf(f, "%lf %lf \n",&options[n].strike,&options[n].ivol);
		options[n].call_price=pnl_bs_call(s, options[n].strike, T, r, 0.0, options[n].ivol/100.);
		options[n].put_price=pnl_bs_put(s, options[n].strike, T, r, 0.0, options[n].ivol/100.);
        if (n>=1)
            if ((options[n-1].strike<=s) && (s<options[n].strike))
                iatm=n-1;
        n++;
    }
    double k0=options[iatm].strike; ////the at-the-money strike price
    double f_price=k0+exp(r*T)*(options[iatm].call_price-options[iatm].put_price); /// the forward index price
    
    printf("ATM strike = %g\n",options[iatm].strike);
    fclose(f);
    
    double price_call[max_options],price_put[max_options],k_call[max_options],k_put[max_options];
    int n_call=0;
    int n_put=0;
    for (int i=0;i<=iatm;i++)
        if (options[i].put_price!=0)
        {
            price_put[n_put]=options[i].put_price;
            k_put[n_put]=options[i].strike;
            n_put++;
        }
    for (int i=iatm+1;i<n;i++)
        if (options[i].call_price!=0)
        {
            price_call[n_call]=options[i].call_price;
            k_call[n_call]=options[i].strike;
            n_call++;
        }
	//////////compute E[ln S_T] //////////////////
    double log_k0=log(k0);
    double sum1=price_call[0]/pow(k_call[0],2)*(k_call[0]-k_put[n_put-1]);
    for (int i=1;i<n_call;i++)
        sum1+=price_call[i]/pow(k_call[i],2)*(k_call[i]-k_call[i-1]);
    double sum2=price_put[0]/pow(k_put[0],2)*(k_put[1]-k_put[0]);
    for (int i=1;i<n_put;i++)
        sum2+=price_put[i]/pow(k_put[i],2)*(k_put[i]-k_put[i-1]);
    double expected=log_k0+f_price/k0-1-exp(r*T)*(sum1+sum2);
	//////////compute E[ln^2 S_T] //////////////////
    sum1=price_call[0]*(1-log(k_call[0]))/pow(k_call[0],2)*(k_call[0]-k_put[n_put-1]);
    for (int i=1;i<n_call;i++)
        sum1+=price_call[i]*(1-log(k_call[i]))/pow(k_call[i],2)*(k_call[i]-k_call[i-1]);
    sum2=price_put[0]*(1-log(k_put[0]))/pow(k_put[0],2)*(k_put[1]-k_put[0]);
    for (int i=1;i<n_put;i++)
        sum2+=price_put[i]*(1-log(k_put[i]))/pow(k_put[i],2)*(k_put[i]-k_put[i-1]);
    double expected_quad=pow(log_k0,2)+2*log_k0*(f_price/k0-1)+2*exp(r*T)*(sum1+sum2);
    double var=expected_quad-pow(expected,2);
    return sqrt(var/T)*100.0;
}
int main(int argc, const char * argv[]) 
{   double Spot=100; // Spot price
	double T=1.; //time to expiry
	double R=10.; // interest rate
	double r=log(1+R/100.);
	double Strike=10.; //Strike price
	double price=0.;


	double vol=levy_vol_product("implied_volatility.dat", Spot, T, r);
	price=vol-Strike*exp(-r*T);

   printf("Fair strike, in annual volatility points: %g\n",vol);
   printf("Volatility swap, in annual volatility points: %g\n",price);

	
return 0;
}
}