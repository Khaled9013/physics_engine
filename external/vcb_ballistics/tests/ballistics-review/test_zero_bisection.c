#include "vcb_ballistics.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int solution_is_zero(const vcb_ballistics_solution_t *solution)
{
    const unsigned char *bytes = (const unsigned char *)solution;
    size_t i;

    for(i = 0u; i < sizeof(*solution); i++){
        if(bytes[i] != 0u){
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    static const float velocities[] = {50.0f, 100.0f, 400.0f, 800.0f, 1500.0f};
    static const float coefficients[] = {0.05f, 0.1f, 0.5f, 1.0f, 2.0f};
    static const float heights[] = {0.0f, 0.1f, 0.2f};
    static const float zero_ranges[] = {10.0f, 100.0f, 1000.0f};
    const vcb_ballistics_environment_t calm = {0.0f, 0.0f};
    float worst_drop_m = 0.0f;
    float worst_elevation_mrad = 0.0f;
    float worst_gate_ratio = 0.0f;
    unsigned int ok_count = 0u;
    unsigned int no_solution_count = 0u;
    unsigned int failures = 0u;
    size_t v;
    size_t b;
    size_t h;
    size_t z;

    for(v = 0u; v < 5u; v++){
        for(b = 0u; b < 5u; b++){
            for(h = 0u; h < 3u; h++){
                for(z = 0u; z < 3u; z++){
                    const vcb_ballistics_profile_t profile = {
                        velocities[v], coefficients[b], heights[h], zero_ranges[z],
                        vcb_ballistics_drag_family_g1
                    };
                    vcb_ballistics_solution_t solution;
                    const vcb_ballistics_status_e status = vcb_ballistics_solve(
                            &profile, &calm, zero_ranges[z], &solution);

                    if(status == vcb_ballistics_status_ok){
                        const float drop_m = fabsf(solution.drop_m);
                        const float elevation_mrad =
                                fabsf(solution.elevation_correction_mrad);
                        /* float angle ULP at 5 deg is 2^-27 rad, wider than the 24-bisection bracket; height = ULP x dy/dtheta ~ range */
                        const float drop_gate_m =
                                1.0e-7f + 3.0e-8f * zero_ranges[z];
                        /* same bound, converted to angle the way the solver converts drop */
                        const float elevation_gate_mrad =
                                drop_gate_m * 1000.0f / zero_ranges[z];
                        const float drop_ratio = drop_m / drop_gate_m;
                        const float elevation_ratio =
                                elevation_mrad / elevation_gate_mrad;

                        ok_count++;
                        if(drop_m > worst_drop_m){
                            worst_drop_m = drop_m;
                        }
                        if(elevation_mrad > worst_elevation_mrad){
                            worst_elevation_mrad = elevation_mrad;
                        }
                        if(drop_ratio > worst_gate_ratio){
                            worst_gate_ratio = drop_ratio;
                        }
                        if(elevation_ratio > worst_gate_ratio){
                            worst_gate_ratio = elevation_ratio;
                        }
                        if(drop_m > drop_gate_m ||
                           elevation_mrad > elevation_gate_mrad){
                            failures++;
                            printf("FAIL v=%g bc=%g h=%g zero=%g drop=%.9g "
                                   "gate=%.9g elevation=%.9g gate=%.9g\n",
                                   (double)velocities[v], (double)coefficients[b],
                                   (double)heights[h], (double)zero_ranges[z],
                                   (double)drop_m, (double)drop_gate_m,
                                   (double)elevation_mrad,
                                   (double)elevation_gate_mrad);
                        }
                    }else if(status == vcb_ballistics_status_no_solution){
                        no_solution_count++;
                        if(!solution_is_zero(&solution)){
                            failures++;
                        }
                    }else{
                        failures++;
                    }
                }
            }
        }
    }

    printf("Zero sweep: 225 cases, %u OK, %u no-solution, %u failures\n",
           ok_count, no_solution_count, failures);
    printf("Zero residual: drop %.9g m, elevation %.9g mrad, worst gate ratio %.4g\n",
           (double)worst_drop_m, (double)worst_elevation_mrad,
           (double)worst_gate_ratio);
    return failures == 0u ? 0 : 1;
}
