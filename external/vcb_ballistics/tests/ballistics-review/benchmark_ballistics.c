#include "vcb_ballistics.h"

#include <stdio.h>
#include <time.h>

int main(void)
{
    const vcb_ballistics_profile_t profile = {1500.0f, 2.0f, 0.2f, 1000.0f, vcb_ballistics_drag_family_g1};
    const vcb_ballistics_environment_t environment = {100.0f, 90.0f};
    vcb_ballistics_solution_t solution;
    volatile float checksum = 0.0f;
    const unsigned int repetitions = 20u;
    clock_t start;
    clock_t end;
    unsigned int i;

    start = clock();
    for(i = 0u; i < repetitions; i++){
        if(vcb_ballistics_solve(&profile, &environment, 2000.0f, &solution) !=
           vcb_ballistics_status_ok){
            return 1;
        }
        checksum += solution.time_of_flight_s;
    }
    end = clock();
    printf("Worst-work host benchmark: %.3f us/solve, checksum %.9g\n",
           1000000.0 * (double)(end - start) /
           ((double)CLOCKS_PER_SEC * (double)repetitions), (double)checksum);
    printf("Output: tof %.9g velocity %.9g drop %.9g drift %.9g elevation %.9g windage %.9g\n",
           (double)solution.time_of_flight_s,
           (double)solution.impact_velocity_mps, (double)solution.drop_m,
           (double)solution.wind_drift_m,
           (double)solution.elevation_correction_mrad,
           (double)solution.windage_correction_mrad);
    return 0;
}
