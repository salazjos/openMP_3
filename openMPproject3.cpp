/*Joseph Salazar
 * salazjos@oregonstate.edu
 * openMPproject3.cpp - Using OpenMP for a month-to-month simulation of agents running in 
 * their own thread and reacting to the state of other agents. The amount the grain 
 * growing is affected by the temperature, amount of precipitation, and the number 
 * of graindeer in the area to eat it. The number of graindeer depends on the amount of 
 * grain available to eat. An introducted agent are hunters that will hunt the deer. 
 * 
*/

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <stdio.h>

/*Units of grain growth are inches.
 Units of temperature are degrees Fahrenheit
 Units of precipitation are inches.*/
const float GRAIN_GROWS_PER_MONTH =        8.0;
const float ONE_DEER_EATS_PER_MONTH =      0.5;

const float AVG_PRECIP_PER_MONTH =         6.0;    // average
const float AMP_PRECIP_PER_MONTH =         6.0;    // plus or minus
const float RANDOM_PRECIP =                2.0;    // plus or minus noise

const float AVG_TEMP =               50.0;    // average
const float AMP_TEMP =               20.0;    // plus or minus
const float RANDOM_TEMP =            10.0;    // plus or minus noise

const float MIDTEMP =                40.0;
const float MIDPRECIP =              10.0;

//MyAgent() constants
const float HUNTER_KILL_PROB[]  = {.10,.20,.30,.40};

//Global state variables for all threads
int    NowYear;        // 2019 - 2024
int    NowMonth;        // 0 - 11

float    NowPrecip;        // inches of rain per month
float    NowTemp;        // temperature this month
float    NowHeight;        // grain height in inches
int      NowNumDeer;        // number of deer in the current population
int      NowDeerHunted;
unsigned int seed = 0;  // a thread-private variable

omp_lock_t    Lock;
int        NumInThreadTeam;
int        NumAtBarrier;
int        NumGone;
// end of Global variables

//function protypes
void    InitBarrier( int );
void    WaitBarrier( );
float   SQR(float);
void    Watcher();
void    Grain();
void    GrainDeer();
void    MyAgent();
void    printCurrentMonthInfo();
void    printNewYearHeader();
float   Ranf( unsigned int *seedp,  float low, float high );
int     Ranf( unsigned int *seedp, int ilow, int ihigh );

int main(int argc, const char * argv[]) {
    
    srand(time(NULL));
    
    //test if OpenMP is supported
    #ifndef _OPENMP
        fprintf( stderr, "No OpenMP support!\n" );
        return 1;
    #endif
    
    // starting month, year, deer amount, grain height
    NowMonth =    0;
    NowYear  = 2019;
    NowNumDeer = 1;
    NowHeight =  1.0;
    NowDeerHunted = 0;
    
    omp_set_num_threads(4); // same as # of sections
    InitBarrier(4);
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            //calculates a new NowNumDeer
            GrainDeer( );
        }
        
        #pragma omp section
        {
            //calculates a new NowHeight
            Grain( );
        }
        
        #pragma omp section
        {
            //calculates a new NowPrecip and NowTemp
            //updates NowYear and NowMonth
            Watcher( );
        }
        
        #pragma omp section
        {
            //hunt deer seasonally 
            MyAgent( );
        }
    }       // implied barrier -- all functions must return in order
    // to allow any of them to get past here
    
    return 0;
}

void Watcher()
{
	//unsigned int seed = 0;
    printNewYearHeader();
    while( NowYear < 2025 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        //. . .
        
        // DoneComputing barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //ASSIGN GLOBAL VARIBLES HERE
        
        // DoneAssigning barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //PRINT RESULTS AND INCREMENT TIME
        printCurrentMonthInfo();
        NowMonth++;
        if(NowMonth > 11){
            NowMonth = 0;
            NowYear++;
            printNewYearHeader();
        }
        //-----------------CALCULATE NEW ENVIROMENTAL VARIABLES---------------
        float ang = (  30.0*(float)NowMonth + 15.0) * (M_PI / 180.0);
        float temp = AVG_TEMP - AMP_TEMP * cos(ang);
        //unsigned int seed = 0;
        NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );
        
        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( &seed, -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0.0)
            NowPrecip = 0.0;
        
        // DonePrinting barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //LOOP BACK TO THE TOP
    }
}

void Grain()
{
    while( NowYear < 2025 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        //. . .
        float grainGrowth = 0.0;
        float tempFactor =   exp( -SQR((NowTemp - MIDTEMP) / 10.));
        float precipFactor = exp( -SQR((NowPrecip - MIDPRECIP) / 10.));
        
        // DoneComputing barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //ASSIGN GLOBAL VARIBLES HERE
        NowHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        NowHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        if(NowHeight < 0)
			NowHeight = 0;
        
        // DoneAssigning barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //Watcher() is printing and incrementing here
        
        // DonePrinting barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //LOOP BACK TO THE TOP
    }
}

void GrainDeer()
{
    while( NowYear < 2025 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        //. . .
        int amountOfDeer = NowNumDeer;
        if(amountOfDeer > NowHeight)
			amountOfDeer--;
		else if(amountOfDeer < NowHeight)
			amountOfDeer++;
			
        // DoneComputing barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //ASSIGN GLOBAL VARIBLES HERE
        NowNumDeer = amountOfDeer;
        
        // DoneAssigning barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //Watcher() is printing and incrementing here
        
        // DonePrinting barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //LOOP BACK TO THE TOP
    }
}

void MyAgent()
{
    while( NowYear < 2025 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        //*******Hunting season is June - Sept
        if(NowMonth >= 5 && NowMonth <= 8){
			int numberOFHunters = (rand()%4)+1;
			//printf("\nHunters: %d\n", numberOFHunters);
			int tempProb = (rand()%4);
			float killProbabilty = HUNTER_KILL_PROB[tempProb];
			NowDeerHunted = (int)(ceil(killProbabilty * numberOFHunters));
		}
        
        // Done Computing barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //ASSIGN GLOBAL VARIBLES HERE
        if(NowMonth >= 5 && NowMonth <= 8){
           NowNumDeer -= NowDeerHunted;
           if(NowNumDeer < 0)
				NowNumDeer = 0;
		}
		else
			NowDeerHunted = 0;
        // DoneAssigning barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //Watcher() is printing and incrementing here
        
        // DonePrinting barrier:
        WaitBarrier(); //-------------------------------------
        //#pragma omp barrier
        //LOOP BACK TO THE TOP
    }
}

void printCurrentMonthInfo()
{
    printf("%d      %3.2f      %3.2f        %3.2f       %d           %d\n",
           NowMonth + 1, NowTemp, NowPrecip,
           NowHeight, NowDeerHunted, NowNumDeer);
}

void printNewYearHeader()
{
    printf("\nYEAR: %d\n", NowYear);
    printf("MONTH   TEMP    PRECIP    GRAIN_HEIGHT    DEER_HUNTED   DEER_AMOUNT\n");
}

//Instructor provided
void InitBarrier( int n )
{
    // specify how many threads will be in the barrier:
    //    (also init's the Lock)
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock( &Lock );
}

//Instructor provided
void WaitBarrier( )
{
    // have the calling thread wait here until all the other threads catch up:
    omp_set_lock( &Lock );
    {
        NumAtBarrier++;
        if( NumAtBarrier == NumInThreadTeam )
        {
            NumGone = 0;
            NumAtBarrier = 0;
            // let all other threads get back to what they were doing
            // before this one unlocks, knowing that they might immediately
            // call WaitBarrier( ) again:
            while( NumGone != NumInThreadTeam-1 );
            omp_unset_lock( &Lock );
            return;
        }
    }
    omp_unset_lock( &Lock );
    
    while( NumAtBarrier != 0 );    // this waits for the nth thread to arrive
    
    #pragma omp atomic
    NumGone++;            // this flags how many threads have returned
}

//Instructor provided
float SQR( float x)
{
    return x * x; //square a number
}

//Instructor provided
float Ranf( unsigned int *seedp,  float low, float high )
{
    float r = (float) rand_r( seedp );              // 0 - RAND_MAX
    return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}

//Instructor provided
int Ranf( unsigned int *seedp, int ilow, int ihigh )
{
    float low = (float)ilow;
    float high = (float)ihigh + 0.9999f;
    return (int)(  Ranf(seedp, low,high) );
}
