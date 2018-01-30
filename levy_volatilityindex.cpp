//
//  main.cpp
//  levy_vol_product
//
//  Created by Alexander Grechko and Oleg Kudryavtsev on 21.07.17.
//  Copyright (c) 2017 Alexander Grechko and Oleg Kudryavtsev. All rights reserved.
// программа подготовлена в рамках проекта №15-32-01390 "Математические методы анализа и управления рисками российского срочного рынка", 
// выполненного при финансовой поддержке РФФИ

#include <vector>
#include <pnl/pnl_cdf.h>


typedef struct option {
    double strike;
    double call_price;
    double put_price;
} option;

//compute volatility index in non-parametric Levy model, fn - file name with option prices
//option strikes in file must be sorted
//s - asset price
//T - time to maturity
double levy_vol_index(const char* fn, double s, double T, double r)
{
    const int max_options=50;
    option options[max_options];
    FILE *f;
	errno_t err;

    //Reading option data from file fn
    
	err=fopen_s(&f, fn, "r");
    if (err>0)
    {
        perror("Could not open file");
        return 0;
    }
    int n=0;
    int iatm=-1; // index of atm put
    //double min_dif=-1;
    //double k0=0;
	////find k0 at-the-money strike price ////////////////
    while (!feof(f))
    {
        fscanf_s(f, "%lf %lf %lf\n",&options[n].strike,&options[n].call_price,&options[n].put_price);
        if (n>=1)
            if ((options[n-1].strike<=s) && (s<options[n].strike))
                iatm=n-1;
        /*
        double dif=fabs(options[n].call_price-options[n].put_price);
        if ((dif<=min_dif) or (min_dif==-1))
        {
            min_dif=dif;
            k0=options[n].strike;
        }*/
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
    double log_k0=log(k0);
    double sum1=price_call[0]/pow(k_call[0],2)*(k_call[0]-k_put[n_put-1]);
    for (int i=1;i<n_call;i++)
        sum1+=price_call[i]/pow(k_call[i],2)*(k_call[i]-k_call[i-1]);
    double sum2=price_put[0]/pow(k_put[0],2)*(k_put[1]-k_put[0]);
    for (int i=1;i<n_put;i++)
        sum2+=price_put[i]/pow(k_put[i],2)*(k_put[i]-k_put[i-1]);
    double expected=log_k0+f_price/k0-1-exp(r*T)*(sum1+sum2);
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
{   double Spot=8276.43;  //Spot price
	double T=0.095186453576; //time to expiry
	double R=0.;  //interest rate
	double r=log(1+R/100.);
	
    double vol=levy_vol_index("option_prices.dat", Spot, T, r);

    printf("Volatility index: %g\n",vol);
    return 0;
}
