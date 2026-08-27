#include "vcb_ballistics.h"

#include <math.h>
#include <stdio.h>

vcb_ballistics_status_e vcb_ballistics_solve_step_025(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);
vcb_ballistics_status_e vcb_ballistics_solve_step_0125(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);
vcb_ballistics_status_e vcb_ballistics_solve_step_00625(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);

/* G7 knot 4 sits at 1110 fps, where fitted retardation jumps +55.4% */
#define VCB_G7_KNOT_MPS 338.328f

/* gates are ~3-5x the measured worst over these crossing geometries */
static const double s_gate[6] = {
    2.0e-3,   /* time of flight, s */
    3.0e-2,   /* impact velocity, m/s */
    5.0e-2,   /* drop, m */
    1.5e-2,   /* wind drift, m */
    5.0e-2,   /* elevation, mrad */
    1.5e-2    /* windage, mrad */
};

typedef struct {
    float ballistic_coefficient;
    float muzzle_velocity_mps;
    float range_m;
} vcb_knot_case_t;

static double vcb_field(const vcb_ballistics_solution_t *s, int index)
{
    switch(index){
    case 0: return (double)s->time_of_flight_s;
    case 1: return (double)s->impact_velocity_mps;
    case 2: return (double)s->drop_m;
    case 3: return (double)s->wind_drift_m;
    case 4: return (double)s->elevation_correction_mrad;
    default: return (double)s->windage_correction_mrad;
    }
}

static int vcb_all_finite(const vcb_ballistics_solution_t *s)
{
    int i;

    for(i = 0; i < 6; i++){
        if(!isfinite(vcb_field(s, i))){
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    static const char *field_name[6] = {"tof_s", "velocity_mps", "drop_m",
                                        "drift_m", "elev_mrad", "wind_mrad"};
    static const vcb_knot_case_t cases[] = {
        {0.05f, 1500.0f, 1200.0f}, {0.05f, 1200.0f, 1000.0f},
        {0.10f, 550.0f, 1200.0f},  {0.10f, 1000.0f, 600.0f},
        {0.20f, 725.0f, 1700.0f},  {0.20f, 1000.0f, 1500.0f},
        {0.30f, 1200.0f, 2000.0f}, {0.50f, 575.0f, 1900.0f},
        {0.50f, 600.0f, 1400.0f},  {0.50f, 400.0f, 1600.0f}
    };
    const size_t case_count = sizeof(cases) / sizeof(cases[0]);
    const vcb_ballistics_environment_t environment = {8.0f, 90.0f};
    double worst[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    unsigned int checks = 0u;
    unsigned int failures = 0u;
    unsigned int crossing_cases = 0u;
    size_t c;
    int f;

    printf("G7 knot crossing at %.3f m/s, step sensitivity 0.5/0.25/0.125 vs "
           "0.0625 m:\n", (double)VCB_G7_KNOT_MPS);
    printf("%-5s %-7s %-6s %8s %10s %10s %10s %10s %10s %10s\n", "bc", "muzzle",
           "range", "impact", field_name[0], field_name[1], field_name[2],
           field_name[3], field_name[4], field_name[5]);

    for(c = 0u; c < case_count; c++){
        const vcb_ballistics_profile_t profile = {
            cases[c].muzzle_velocity_mps, cases[c].ballistic_coefficient,
            0.05f, 100.0f, vcb_ballistics_drag_family_g7
        };
        vcb_ballistics_solution_t production;
        vcb_ballistics_solution_t step_025;
        vcb_ballistics_solution_t step_0125;
        vcb_ballistics_solution_t reference;
        double delta[6];

        checks++;
        if(vcb_ballistics_solve(&profile, &environment, cases[c].range_m,
                                &production) != vcb_ballistics_status_ok ||
           vcb_ballistics_solve_step_025(&profile, &environment, cases[c].range_m,
                                         &step_025) != vcb_ballistics_status_ok ||
           vcb_ballistics_solve_step_0125(&profile, &environment, cases[c].range_m,
                                          &step_0125) != vcb_ballistics_status_ok ||
           vcb_ballistics_solve_step_00625(&profile, &environment, cases[c].range_m,
                                           &reference) != vcb_ballistics_status_ok){
            printf("FAIL case %zu did not solve at all four step sizes\n", c);
            failures++;
            continue;
        }

        /* the case is only meaningful if the trajectory really traverses the knot */
        checks++;
        if(production.impact_velocity_mps < VCB_G7_KNOT_MPS &&
           cases[c].muzzle_velocity_mps > VCB_G7_KNOT_MPS){
            crossing_cases++;
        }else{
            printf("FAIL case %zu does not cross the knot (impact %.2f)\n", c,
                   (double)production.impact_velocity_mps);
            failures++;
        }

        checks++;
        if(!vcb_all_finite(&production) || !vcb_all_finite(&step_025) ||
           !vcb_all_finite(&step_0125) || !vcb_all_finite(&reference)){
            printf("FAIL case %zu produced a non-finite output\n", c);
            failures++;
        }

        for(f = 0; f < 6; f++){
            const double ref = vcb_field(&reference, f);
            double d = fabs(vcb_field(&production, f) - ref);
            double t = fabs(vcb_field(&step_025, f) - ref);

            if(t > d){ d = t; }
            t = fabs(vcb_field(&step_0125, f) - ref);
            if(t > d){ d = t; }
            delta[f] = d;
            if(d > worst[f]){ worst[f] = d; }
            checks++;
            if(!(d <= s_gate[f])){
                failures++;
                printf("FAIL case %zu %s delta %.9g exceeds %.9g\n", c,
                       field_name[f], d, s_gate[f]);
            }
        }
        printf("%-5.2f %-7.0f %-6.0f %8.2f %10.4g %10.4g %10.4g %10.4g %10.4g %10.4g\n",
               (double)cases[c].ballistic_coefficient,
               (double)cases[c].muzzle_velocity_mps, (double)cases[c].range_m,
               (double)production.impact_velocity_mps, delta[0], delta[1],
               delta[2], delta[3], delta[4], delta[5]);
    }

    /* no artifact near the knot: statuses defined, outputs finite, drop monotone */
    {
        unsigned int swept = 0u;
        unsigned int monotonicity_breaks = 0u;
        double previous_drop = 1.0e30;
        float muzzle;

        for(muzzle = 600.0f; muzzle <= 1400.0f; muzzle += 1.0f){
            const vcb_ballistics_profile_t profile = {
                muzzle, 0.2f, 0.05f, 100.0f, vcb_ballistics_drag_family_g7
            };
            vcb_ballistics_solution_t solution;
            const vcb_ballistics_status_e status = vcb_ballistics_solve(
                    &profile, &environment, 1200.0f, &solution);

            checks++;
            if(status != vcb_ballistics_status_ok &&
               status != vcb_ballistics_status_no_solution){
                failures++;
                printf("FAIL sweep muzzle %.0f undefined status %d\n",
                       (double)muzzle, (int)status);
                continue;
            }
            if(status != vcb_ballistics_status_ok){
                continue;
            }
            swept++;
            checks++;
            if(!vcb_all_finite(&solution)){
                failures++;
                printf("FAIL sweep muzzle %.0f non-finite output\n",
                       (double)muzzle);
            }
            /* raising muzzle velocity may not increase drop at a fixed range */
            checks++;
            if((double)solution.drop_m > previous_drop + 1.0e-6){
                failures++;
                monotonicity_breaks++;
                printf("FAIL sweep muzzle %.0f drop %.6f rose above %.6f\n",
                       (double)muzzle, (double)solution.drop_m, previous_drop);
            }
            previous_drop = (double)solution.drop_m;
        }
        printf("\nKnot sweep: %u solved muzzle points, %u monotonicity breaks\n",
               swept, monotonicity_breaks);
    }

    printf("Worst deltas over %u crossing cases:", crossing_cases);
    for(f = 0; f < 6; f++){
        printf(" %s=%.6g", field_name[f], worst[f]);
    }
    printf("\nG7 knot: %u checks, %u failures\n", checks, failures);
    return failures == 0u ? 0 : 1;
}
