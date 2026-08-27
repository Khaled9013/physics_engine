#include "vcb_ballistics.h"

#include <float.h>
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

/* measured worst step sensitivity is 5.32e-06 = 45 x FLT_EPSILON; gate at 3.8x that */
#define VCB_STEP_NOISE_GATE 2.0e-5

typedef struct {
    const char *name;
    vcb_ballistics_profile_t profile;
    vcb_ballistics_environment_t environment;
    float range_m;
} vcb_order_case_t;

static double vcb_field(const vcb_ballistics_solution_t *solution, int index)
{
    switch(index){
    case 0: return (double)solution->time_of_flight_s;
    case 1: return (double)solution->impact_velocity_mps;
    case 2: return (double)solution->drop_m;
    default: return (double)solution->wind_drift_m;
    }
}

int main(void)
{
    static const char *field_name[4] = {"tof", "velocity", "drop", "drift"};
    static const vcb_order_case_t cases[] = {
        {"short",     {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},  {5.0f, 90.0f},  200.0f},
        {"medium",    {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},  {5.0f, 90.0f},  800.0f},
        {"long",      {900.0f, 0.6f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},  {10.0f, 90.0f}, 2000.0f},
        {"slow",      {300.0f, 0.3f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},  {5.0f, 45.0f},  600.0f},
        {"transonic", {420.0f, 0.4f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},  {8.0f, 90.0f},  900.0f},
        {"highdrag",  {700.0f, 0.1f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},  {10.0f, 60.0f}, 700.0f},
        {"fast",      {1200.0f, 1.0f, 0.05f, 200.0f, vcb_ballistics_drag_family_g1}, {6.0f, 90.0f},  1500.0f}
    };
    const size_t case_count = sizeof(cases) / sizeof(cases[0]);
    unsigned int checks = 0u;
    unsigned int failures = 0u;
    double worst_relative = 0.0;
    const char *worst_case = "none";
    const char *worst_field = "none";
    size_t c;
    int f;

    printf("Step sensitivity relative to the 0.0625 m reference build:\n");
    printf("%-10s %-9s %13s %11s %11s %11s\n", "case", "field", "reference",
           "rel(0.5)", "rel(0.25)", "rel(0.125)");

    for(c = 0u; c < case_count; c++){
        vcb_ballistics_solution_t production;
        vcb_ballistics_solution_t step_025;
        vcb_ballistics_solution_t step_0125;
        vcb_ballistics_solution_t reference;

        if(vcb_ballistics_solve(&cases[c].profile, &cases[c].environment,
                                cases[c].range_m,
                                &production) != vcb_ballistics_status_ok ||
           vcb_ballistics_solve_step_025(&cases[c].profile, &cases[c].environment,
                                         cases[c].range_m,
                                         &step_025) != vcb_ballistics_status_ok ||
           vcb_ballistics_solve_step_0125(&cases[c].profile, &cases[c].environment,
                                          cases[c].range_m,
                                          &step_0125) != vcb_ballistics_status_ok ||
           vcb_ballistics_solve_step_00625(&cases[c].profile, &cases[c].environment,
                                           cases[c].range_m,
                                           &reference) != vcb_ballistics_status_ok){
            printf("%-10s SOLVE FAILED\n", cases[c].name);
            failures++;
            continue;
        }

        for(f = 0; f < 4; f++){
            const double ref = vcb_field(&reference, f);
            /* floor the scale so near-zero fields do not divide by noise */
            const double scale = fabs(ref) > 1.0e-3 ? fabs(ref) : 1.0e-3;
            const double rel[3] = {
                fabs(vcb_field(&production, f) - ref) / scale,
                fabs(vcb_field(&step_025, f) - ref) / scale,
                fabs(vcb_field(&step_0125, f) - ref) / scale
            };
            int i;

            printf("%-10s %-9s %13.6g %11.4g %11.4g %11.4g\n", cases[c].name,
                   field_name[f], ref, rel[0], rel[1], rel[2]);
            for(i = 0; i < 3; i++){
                checks++;
                if(rel[i] > worst_relative){
                    worst_relative = rel[i];
                    worst_case = cases[c].name;
                    worst_field = field_name[f];
                }
                if(!(rel[i] <= VCB_STEP_NOISE_GATE)){
                    failures++;
                    printf("FAIL %s %s step index %d relative %.9g exceeds %.9g\n",
                           cases[c].name, field_name[f], i, rel[i],
                           (double)VCB_STEP_NOISE_GATE);
                }
            }
        }
    }

    printf("\nWorst relative step sensitivity %.9g at %s/%s, gate %.9g, "
           "FLT_EPSILON %.9g\n", worst_relative, worst_case, worst_field,
           (double)VCB_STEP_NOISE_GATE, (double)FLT_EPSILON);
    printf("Convergence order: %u checks, %u failures\n", checks, failures);
    return failures == 0u ? 0 : 1;
}
