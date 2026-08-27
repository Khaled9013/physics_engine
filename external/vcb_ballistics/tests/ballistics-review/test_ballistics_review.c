#include "vcb_ballistics.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

vcb_ballistics_status_e vcb_ballistics_solve_step_025(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);
vcb_ballistics_status_e vcb_ballistics_solve_step_0125(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);
vcb_ballistics_status_e vcb_ballistics_solve_step_00625(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);

typedef vcb_ballistics_status_e (*solve_fn_t)(
        const vcb_ballistics_profile_t *, const vcb_ballistics_environment_t *,
        float, vcb_ballistics_solution_t *);

typedef struct {
    const char *name;
    vcb_ballistics_profile_t profile;
    vcb_ballistics_environment_t environment;
    float range_m;
} trajectory_case_t;

static const solve_fn_t s_solvers[] = {
    vcb_ballistics_solve, vcb_ballistics_solve_step_025,
    vcb_ballistics_solve_step_0125, vcb_ballistics_solve_step_00625
};
static const vcb_ballistics_profile_t s_nominal_profile = {
    800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1
};
static const vcb_ballistics_environment_t s_calm = {0.0f, 0.0f};
static unsigned int s_checks;
static unsigned int s_failures;

static void check_true(int condition, const char *name)
{
    s_checks++;
    if(!condition){
        s_failures++;
        printf("FAIL %s\n", name);
    }
}

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

static int solution_is_finite(const vcb_ballistics_solution_t *solution)
{
    return isfinite(solution->time_of_flight_s) &&
           isfinite(solution->impact_velocity_mps) && isfinite(solution->drop_m) &&
           isfinite(solution->wind_drift_m) &&
           isfinite(solution->elevation_correction_mrad) &&
           isfinite(solution->windage_correction_mrad);
}

static void poison_solution(vcb_ballistics_solution_t *solution)
{
    memset(solution, 0xa5, sizeof(*solution));
}

static void expect_failure(vcb_ballistics_status_e expected,
                           const vcb_ballistics_profile_t *profile,
                           const vcb_ballistics_environment_t *environment,
                           float range_m, const char *name)
{
    vcb_ballistics_solution_t solution;
    vcb_ballistics_status_e status;

    poison_solution(&solution);
    status = vcb_ballistics_solve(profile, environment, range_m, &solution);
    check_true(status == expected, name);
    check_true(solution_is_zero(&solution), "failure output cleared");
}

static void expect_accepted(const vcb_ballistics_profile_t *profile,
                            const vcb_ballistics_environment_t *environment,
                            float range_m, const char *name)
{
    vcb_ballistics_solution_t solution;
    const vcb_ballistics_status_e status = vcb_ballistics_solve(
            profile, environment, range_m, &solution);

    check_true(status != vcb_ballistics_status_invalid_argument &&
               status != vcb_ballistics_status_out_of_range, name);
    check_true(status == vcb_ballistics_status_ok ? solution_is_finite(&solution) :
               solution_is_zero(&solution), "accepted output invariant");
}

static void set_field(vcb_ballistics_profile_t *profile,
                      vcb_ballistics_environment_t *environment,
                      float *range_m, unsigned int field, float value)
{
    switch(field){
        case 0u: profile->muzzle_velocity_mps = value; break;
        case 1u: profile->ballistic_coefficient = value; break;
        case 2u: profile->sight_height_m = value; break;
        case 3u: profile->zero_range_m = value; break;
        case 4u: environment->wind_speed_mps = value; break;
        case 5u: environment->wind_direction_deg = value; break;
        default: *range_m = value; break;
    }
}

static void test_arguments_and_bounds(void)
{
    static const float invalid[] = {NAN, INFINITY, -INFINITY};
    static const float minimum[] = {50.0f, 0.05f, 0.0f, 10.0f, 0.0f, 0.0f, 1.0f};
    static const float maximum[] = {1500.0f, 2.0f, 0.2f, 1000.0f,
                                    100.0f, 360.0f, 2000.0f};
    unsigned int field;
    size_t value;

    expect_failure(vcb_ballistics_status_invalid_argument, NULL, &s_calm,
                   100.0f, "null profile");
    expect_failure(vcb_ballistics_status_invalid_argument, &s_nominal_profile,
                   NULL, 100.0f, "null environment");
    check_true(vcb_ballistics_solve(&s_nominal_profile, &s_calm, 100.0f, NULL) ==
               vcb_ballistics_status_invalid_argument, "null solution");

    for(field = 0u; field < 7u; field++){
        for(value = 0u; value < sizeof(invalid) / sizeof(invalid[0]); value++){
            vcb_ballistics_profile_t profile = s_nominal_profile;
            vcb_ballistics_environment_t environment = s_calm;
            float range_m = 100.0f;

            set_field(&profile, &environment, &range_m, field, invalid[value]);
            expect_failure(vcb_ballistics_status_invalid_argument, &profile,
                           &environment, range_m, "NaN or infinity rejected");
        }
    }

    for(field = 0u; field < 7u; field++){
        vcb_ballistics_profile_t profile = s_nominal_profile;
        vcb_ballistics_environment_t environment = s_calm;
        float range_m = 100.0f;

        set_field(&profile, &environment, &range_m, field, minimum[field]);
        expect_accepted(&profile, &environment, range_m, "exact minimum accepted");
        set_field(&profile, &environment, &range_m, field,
                  nextafterf(minimum[field], -INFINITY));
        expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                       range_m, "below minimum rejected");

        profile = s_nominal_profile;
        environment = s_calm;
        range_m = 100.0f;
        if(field == 5u){
            set_field(&profile, &environment, &range_m, field,
                      nextafterf(maximum[field], 0.0f));
            expect_accepted(&profile, &environment, range_m,
                            "below exclusive maximum accepted");
            set_field(&profile, &environment, &range_m, field, maximum[field]);
            expect_failure(vcb_ballistics_status_out_of_range, &profile,
                           &environment, range_m, "exclusive maximum rejected");
        }else{
            set_field(&profile, &environment, &range_m, field, maximum[field]);
            expect_accepted(&profile, &environment, range_m,
                            "exact maximum accepted");
            set_field(&profile, &environment, &range_m, field,
                      nextafterf(maximum[field], INFINITY));
            expect_failure(vcb_ballistics_status_out_of_range, &profile,
                           &environment, range_m, "above maximum rejected");
        }
    }

    expect_accepted(&s_nominal_profile,
                    &(vcb_ballistics_environment_t){-0.0f, -0.0f},
                    100.0f, "signed zero accepted");
    expect_failure(vcb_ballistics_status_no_solution,
                   &(vcb_ballistics_profile_t){50.0f, 0.05f, 0.2f, 1000.0f, vcb_ballistics_drag_family_g1},
                   &s_calm, 2000.0f, "unreachable trajectory");
}

static float relative_delta(float actual, float reference)
{
    return fabsf(actual - reference) / fmaxf(fabsf(reference), 1.0e-6f);
}

static float solution_component(const vcb_ballistics_solution_t *solution,
                                size_t index)
{
    float values[6];

    values[0] = solution->time_of_flight_s;
    values[1] = solution->impact_velocity_mps;
    values[2] = solution->drop_m;
    values[3] = solution->wind_drift_m;
    values[4] = solution->elevation_correction_mrad;
    values[5] = solution->windage_correction_mrad;
    return values[index];
}

static void test_wind_and_repeatability(void)
{
    static const float directions[] = {0.0f, 45.0f, 90.0f, 135.0f,
                                       180.0f, 225.0f, 270.0f, 315.0f};
    vcb_ballistics_solution_t solution[8];
    vcb_ballistics_solution_t repeat;
    vcb_ballistics_solution_t positive_zero;
    vcb_ballistics_solution_t negative_zero;
    size_t i;

    for(i = 0u; i < 8u; i++){
        const vcb_ballistics_environment_t environment = {100.0f, directions[i]};
        check_true(vcb_ballistics_solve(&s_nominal_profile, &environment, 800.0f,
                                        &solution[i]) == vcb_ballistics_status_ok,
                   "wind solve");
        check_true(solution_is_finite(&solution[i]), "wind output finite");
    }
    check_true(fabsf(solution[0].wind_drift_m) < 1.0e-5f,
               "tailwind has zero lateral drift");
    check_true(fabsf(solution[4].wind_drift_m) < 1.0e-4f,
               "headwind has zero lateral drift");
    check_true(solution[2].wind_drift_m > 0.0f &&
               solution[2].windage_correction_mrad < 0.0f,
               "right wind sign contract");
    check_true(solution[6].wind_drift_m < 0.0f &&
               solution[6].windage_correction_mrad > 0.0f,
               "left wind sign contract");
    check_true(relative_delta(solution[2].wind_drift_m,
                              -solution[6].wind_drift_m) < 2.0e-5f,
               "crosswind mirror");
    check_true(relative_delta(solution[1].wind_drift_m,
                              -solution[7].wind_drift_m) < 2.0e-5f,
               "tail oblique mirror");
    check_true(relative_delta(solution[3].wind_drift_m,
                              -solution[5].wind_drift_m) < 2.0e-5f,
               "head oblique mirror");
    check_true(solution[0].time_of_flight_s < solution[4].time_of_flight_s,
               "tailwind time below headwind");
    check_true(fabsf(solution[1].wind_drift_m) < solution[2].wind_drift_m,
               "oblique drift below crosswind");

    check_true(vcb_ballistics_solve(&s_nominal_profile,
                    &(vcb_ballistics_environment_t){100.0f, 90.0f}, 800.0f,
                    &repeat) == vcb_ballistics_status_ok, "repeat status");
    check_true(memcmp(&solution[2], &repeat, sizeof(repeat)) == 0,
               "bitwise repeatability");
    check_true(vcb_ballistics_solve(&s_nominal_profile,
                    &(vcb_ballistics_environment_t){0.0f, 0.0f}, 800.0f,
                    &positive_zero) == vcb_ballistics_status_ok,
               "positive zero solve");
    check_true(vcb_ballistics_solve(&s_nominal_profile,
                    &(vcb_ballistics_environment_t){-0.0f, -0.0f}, 800.0f,
                    &negative_zero) == vcb_ballistics_status_ok,
               "negative zero solve");
    check_true(memcmp(&positive_zero, &negative_zero, sizeof(positive_zero)) == 0,
               "signed-zero equivalence");

    for(i = 0u; i < 2u; i++){
        const vcb_ballistics_environment_t environment = {
            100.0f, i == 0u ? 0.0f : 180.0f
        };
        vcb_ballistics_solution_t extreme;
        const vcb_ballistics_status_e status = vcb_ballistics_solve(
                &(vcb_ballistics_profile_t){50.0f, 2.0f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1},
                &environment, 100.0f, &extreme);

        check_true(status == vcb_ballistics_status_ok ||
                   status == vcb_ballistics_status_no_solution,
                   "extreme axial wind status");
        check_true(status == vcb_ballistics_status_ok ?
                   solution_is_finite(&extreme) : solution_is_zero(&extreme),
                   "extreme axial wind invariant");
    }
}

static void test_convergence(void)
{
    static const trajectory_case_t cases[] = {
        {"tiny", {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, {0.0f, 0.0f}, 1.0f},
        {"zero", {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, {0.0f, 0.0f}, 100.0f},
        {"nominal", {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, {5.0f, 90.0f}, 500.0f},
        {"oblique", {900.0f, 0.7f, 0.03f, 200.0f, vcb_ballistics_drag_family_g1}, {25.0f, 135.0f}, 1200.0f},
        {"long", {1100.0f, 1.2f, 0.08f, 300.0f, vcb_ballistics_drag_family_g1}, {25.0f, 315.0f}, 2000.0f},
        {"transonic", {400.0f, 0.4f, 0.04f, 100.0f, vcb_ballistics_drag_family_g1}, {10.0f, 45.0f}, 600.0f},
        {"high-drag", {500.0f, 0.05f, 0.02f, 50.0f, vcb_ballistics_drag_family_g1}, {10.0f, 270.0f}, 200.0f},
        {"max-wind", {1000.0f, 0.8f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, {100.0f, 225.0f}, 1000.0f}
    };
    static const char *names[] = {
        "time_s", "velocity_mps", "drop_m", "drift_m",
        "elevation_mrad", "windage_mrad"
    };
    float worst_abs[3][6] = {{0.0f}};
    float worst_rel[3][6] = {{0.0f}};
    const char *worst_case[3][6] = {{NULL}};
    size_t c;

    for(c = 0u; c < sizeof(cases) / sizeof(cases[0]); c++){
        vcb_ballistics_solution_t solution[4];
        vcb_ballistics_status_e status[4];
        size_t solver;
        size_t output;

        for(solver = 0u; solver < 4u; solver++){
            status[solver] = s_solvers[solver](&cases[c].profile,
                    &cases[c].environment, cases[c].range_m, &solution[solver]);
        }
        for(solver = 0u; solver < 4u; solver++){
            check_true(status[solver] == status[3], "convergence status agreement");
            check_true(status[solver] == vcb_ballistics_status_ok ?
                       solution_is_finite(&solution[solver]) :
                       solution_is_zero(&solution[solver]),
                       "convergence output invariant");
        }
        if(status[3] != vcb_ballistics_status_ok){
            continue;
        }
        for(solver = 0u; solver < 3u; solver++){
            for(output = 0u; output < 6u; output++){
                const float actual = solution_component(&solution[solver], output);
                const float reference = solution_component(&solution[3], output);
                const float absolute = fabsf(actual - reference);

                if(absolute > worst_abs[solver][output]){
                    worst_abs[solver][output] = absolute;
                    worst_rel[solver][output] = relative_delta(actual, reference);
                    worst_case[solver][output] = cases[c].name;
                }
            }
        }
    }

    for(c = 0u; c < 3u; c++){
        size_t output;

        printf("Convergence solver step index %zu vs 0.0625 m:\n", c);
        for(output = 0u; output < 6u; output++){
            printf("  %s abs=%.9g rel=%.9g case=%s\n", names[output],
                   worst_abs[c][output], worst_rel[c][output],
                   worst_case[c][output] == NULL ? "none" : worst_case[c][output]);
        }
    }
    check_true(worst_abs[0][0] < 2.0e-4f, "0.5m time convergence");
    check_true(worst_abs[0][1] < 0.2f, "0.5m velocity convergence");
    check_true(worst_abs[0][2] < 0.01f, "0.5m drop convergence");
    check_true(worst_abs[0][3] < 0.01f, "0.5m drift convergence");
    check_true(worst_abs[0][4] < 0.01f, "0.5m elevation convergence");
    check_true(worst_abs[0][5] < 0.01f, "0.5m windage convergence");
}

static void test_matrix(void)
{
    static const vcb_ballistics_profile_t profiles[] = {
        {50.0f, 2.0f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1}, {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1},
        {1500.0f, 2.0f, 0.2f, 1000.0f, vcb_ballistics_drag_family_g1}
    };
    static const float ranges[] = {1.0f, 100.0f, 1000.0f, 2000.0f, vcb_ballistics_drag_family_g1};
    static const float speeds[] = {0.0f, 100.0f};
    static const float directions[] = {0.0f, 45.0f, 90.0f, 135.0f,
                                       180.0f, 225.0f, 270.0f, 315.0f};
    unsigned int ok_count = 0u;
    unsigned int no_solution_count = 0u;
    size_t p;
    size_t r;
    size_t w;
    size_t d;

    for(p = 0u; p < 3u; p++){
        for(r = 0u; r < 4u; r++){
            for(w = 0u; w < 2u; w++){
                for(d = 0u; d < 8u; d++){
                    const vcb_ballistics_environment_t environment = {
                        speeds[w], directions[d]
                    };
                    vcb_ballistics_solution_t solution[4];
                    vcb_ballistics_status_e status[4];
                    size_t solver;

                    for(solver = 0u; solver < 4u; solver++){
                        status[solver] = s_solvers[solver](&profiles[p], &environment,
                                                          ranges[r], &solution[solver]);
                    }
                    for(solver = 0u; solver < 4u; solver++){
                        check_true(status[solver] == status[0],
                                   "matrix step status agreement");
                        check_true(status[solver] == vcb_ballistics_status_ok ?
                                   solution_is_finite(&solution[solver]) :
                                   solution_is_zero(&solution[solver]),
                                   "matrix finite or cleared invariant");
                    }
                    if(status[0] == vcb_ballistics_status_ok){
                        ok_count++;
                    }else if(status[0] == vcb_ballistics_status_no_solution){
                        no_solution_count++;
                    }else{
                        check_true(0, "matrix status class");
                    }
                }
            }
        }
    }
    printf("Cross-parameter matrix: %u cases, %u OK, %u no-solution\n",
           ok_count + no_solution_count, ok_count, no_solution_count);
}

int main(void)
{
    test_arguments_and_bounds();
    test_wind_and_repeatability();
    test_convergence();
    test_matrix();
    printf("Ballistics review: %u checks, %u failures\n", s_checks, s_failures);
    return s_failures == 0u ? 0 : 1;
}
