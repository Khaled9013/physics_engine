#include "vcb_ballistics.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char case_id[40];
    float range_m;
    vcb_ballistics_solution_t expected;
} reference_row_t;

typedef struct {
    float time_abs;
    float time_rel;
    float velocity_abs;
    float velocity_rel;
    float drop_abs;
    float drop_rel;
    float drift_abs;
    float drift_rel;
    float angle_abs;
    float angle_rel;
} tolerance_t;

static unsigned s_checks;
static unsigned s_failures;

static const vcb_ballistics_profile_t s_nominal_profile = {
    .muzzle_velocity_mps = 800.0f,
    .ballistic_coefficient = 0.5f,
    .sight_height_m = 0.05f,
    .zero_range_m = 100.0f,
    .drag_family = vcb_ballistics_drag_family_g1,
};

/* Frozen 2026-08-18 G7 fixture profile; see docs/ballistics-validation-reference.md. */
static const vcb_ballistics_profile_t s_g7_profile = {
    .muzzle_velocity_mps = 800.0f,
    .ballistic_coefficient = 0.25f,
    .sight_height_m = 0.05f,
    .zero_range_m = 100.0f,
    .drag_family = vcb_ballistics_drag_family_g7,
};

static const vcb_ballistics_environment_t s_nominal_wind = {
    .wind_speed_mps = 5.0f,
    .wind_direction_deg = 90.0f,
};

static void check_true(bool condition, const char *name){
    s_checks++;
    if(!condition){
        s_failures++;
        fprintf(stderr, "FAIL: %s\n", name);
    }
}

static bool nearly_equal(float actual, float expected, float abs_tol, float rel_tol){
    const float limit = fmaxf(abs_tol, fabsf(expected) * rel_tol);
    return isfinite(actual) && fabsf(actual - expected) <= limit;
}

static bool solution_is_zero(const vcb_ballistics_solution_t *solution){
    return solution->time_of_flight_s == 0.0f &&
           solution->impact_velocity_mps == 0.0f &&
           solution->drop_m == 0.0f &&
           solution->wind_drift_m == 0.0f &&
           solution->elevation_correction_mrad == 0.0f &&
           solution->windage_correction_mrad == 0.0f;
}

static void poison_solution(vcb_ballistics_solution_t *solution){
    solution->time_of_flight_s = 1.0f;
    solution->impact_velocity_mps = 2.0f;
    solution->drop_m = 3.0f;
    solution->wind_drift_m = 4.0f;
    solution->elevation_correction_mrad = 5.0f;
    solution->windage_correction_mrad = 6.0f;
}

static void expect_failure(vcb_ballistics_status_e expected_status,
                           const vcb_ballistics_profile_t *profile,
                           const vcb_ballistics_environment_t *environment,
                           float range_m,
                           const char *name){
    vcb_ballistics_solution_t solution;
    poison_solution(&solution);
    const vcb_ballistics_status_e status =
        vcb_ballistics_solve(profile, environment, range_m, &solution);
    check_true(status == expected_status, name);
    check_true(solution_is_zero(&solution), "failure clears output");
}

static const tolerance_t *tolerance_for(const char *case_id){
    static const tolerance_t constant_g1 = {
        .time_abs = 0.010f,
        .time_rel = 0.0f,
        .velocity_abs = 2.0f,
        .velocity_rel = 0.0f,
        .drop_abs = 0.02f,
        .drop_rel = 0.0f,
        .drift_abs = 0.01f,
        .drift_rel = 0.0f,
        .angle_abs = 0.03f,
        .angle_rel = 0.0f,
    };
    /* Frozen 2026-08-18 before any G7 solver contact. */
    static const tolerance_t constant_g7 = {
        .time_abs = 0.010f,
        .time_rel = 0.0f,
        .velocity_abs = 3.0f,
        .velocity_rel = 0.0f,
        .drop_abs = 0.030f,
        .drop_rel = 0.0f,
        .drift_abs = 0.015f,
        .drift_rel = 0.0f,
        .angle_abs = 0.040f,
        .angle_rel = 0.0f,
    };
    static const tolerance_t swiss_p = {
        .time_abs = 0.040f,
        .time_rel = 0.08f,
        .velocity_abs = 35.0f,
        .velocity_rel = 0.08f,
        .drop_abs = 0.20f,
        .drop_rel = 0.12f,
        .drift_abs = 0.25f,
        .drift_rel = 0.15f,
        .angle_abs = 0.50f,
        .angle_rel = 0.15f,
    };
    if(strcmp(case_id, "constant_g1_0500") == 0){
        return &constant_g1;
    }
    if(strcmp(case_id, "constant_g7_0250") == 0){
        return &constant_g7;
    }
    return &swiss_p;
}

static bool profile_for(const char *case_id,
                        vcb_ballistics_profile_t *profile,
                        vcb_ballistics_environment_t *environment){
    *environment = s_nominal_wind;
    if(strcmp(case_id, "constant_g1_0500") == 0){
        *profile = s_nominal_profile;
        return true;
    }
    if(strcmp(case_id, "constant_g7_0250") == 0){
        *profile = s_g7_profile;
        return true;
    }
    if(strcmp(case_id, "swiss_p_308_final_sr") == 0){
        profile->muzzle_velocity_mps = 895.0f;
        profile->ballistic_coefficient = 0.2397f;
        profile->sight_height_m = 0.020f;
        profile->zero_range_m = 100.0f;
        profile->drag_family = vcb_ballistics_drag_family_g1;
        return true;
    }
    return false;
}

static bool parse_reference_row(const char *line, reference_row_t *row){
    return sscanf(line, "%39[^,],%f,%f,%f,%f,%f,%f,%f",
                  row->case_id,
                  &row->range_m,
                  &row->expected.time_of_flight_s,
                  &row->expected.impact_velocity_mps,
                  &row->expected.drop_m,
                  &row->expected.wind_drift_m,
                  &row->expected.elevation_correction_mrad,
                  &row->expected.windage_correction_mrad) == 8;
}

static void check_reference_field(const char *case_id,
                                  float range_m,
                                  const char *field,
                                  float actual,
                                  float expected,
                                  float abs_tol,
                                  float rel_tol){
    char name[160];
    snprintf(name, sizeof(name), "%s %.2f m %s", case_id, range_m, field);
    check_true(nearly_equal(actual, expected, abs_tol, rel_tol), name);
}

static void check_reference_row(const reference_row_t *row){
    vcb_ballistics_profile_t profile;
    vcb_ballistics_environment_t environment;
    vcb_ballistics_solution_t actual;
    const tolerance_t *tolerance = tolerance_for(row->case_id);
    const bool gates_vertical = strcmp(row->case_id, "swiss_p_308_final_sr") != 0;
    check_true(profile_for(row->case_id, &profile, &environment), "known fixture case");
    if(!profile_for(row->case_id, &profile, &environment)){
        return;
    }
    const vcb_ballistics_status_e status =
        vcb_ballistics_solve(&profile, &environment, row->range_m, &actual);
    check_true(status == vcb_ballistics_status_ok, "fixture solve status");
    if(status != vcb_ballistics_status_ok){
        return;
    }

    printf("REF %-24s %7.2f m tof %.6f/%.6f vel %.3f/%.3f "
           "drop %.6f/%.6f drift %.6f/%.6f elev %.6f/%.6f wind %.6f/%.6f\n",
           row->case_id, row->range_m,
           actual.time_of_flight_s, row->expected.time_of_flight_s,
           actual.impact_velocity_mps, row->expected.impact_velocity_mps,
           actual.drop_m, row->expected.drop_m,
           actual.wind_drift_m, row->expected.wind_drift_m,
           actual.elevation_correction_mrad,
           row->expected.elevation_correction_mrad,
           actual.windage_correction_mrad,
           row->expected.windage_correction_mrad);

    check_reference_field(row->case_id, row->range_m, "time",
                          actual.time_of_flight_s, row->expected.time_of_flight_s,
                          tolerance->time_abs, tolerance->time_rel);
    check_reference_field(row->case_id, row->range_m, "velocity",
                          actual.impact_velocity_mps, row->expected.impact_velocity_mps,
                          tolerance->velocity_abs, tolerance->velocity_rel);
    if(gates_vertical){
        check_reference_field(row->case_id, row->range_m, "drop",
                              actual.drop_m, row->expected.drop_m,
                              tolerance->drop_abs, tolerance->drop_rel);
    }
    check_reference_field(row->case_id, row->range_m, "drift",
                          actual.wind_drift_m, row->expected.wind_drift_m,
                          tolerance->drift_abs, tolerance->drift_rel);
    if(gates_vertical){
        check_reference_field(row->case_id, row->range_m, "elevation",
                              actual.elevation_correction_mrad,
                              row->expected.elevation_correction_mrad,
                              tolerance->angle_abs, tolerance->angle_rel);
    }
    check_reference_field(row->case_id, row->range_m, "windage",
                          actual.windage_correction_mrad,
                          row->expected.windage_correction_mrad,
                          tolerance->angle_abs, tolerance->angle_rel);
}

static void test_reference_cases(const char *path, unsigned expected_rows){
    FILE *file = fopen(path, "r");
    check_true(file != NULL, "reference CSV opens");
    if(file == NULL){
        return;
    }
    char line[512];
    check_true(fgets(line, sizeof(line), file) != NULL, "reference CSV header");
    unsigned rows = 0u;
    while(fgets(line, sizeof(line), file) != NULL){
        reference_row_t row;
        check_true(parse_reference_row(line, &row), "reference CSV row parses");
        if(parse_reference_row(line, &row)){
            check_reference_row(&row);
            rows++;
        }
    }
    check_true(ferror(file) == 0, "reference CSV read cleanly");
    fclose(file);
    check_true(rows == expected_rows, "reference CSV row count");
}

static void test_null_and_nonfinite_inputs(void){
    vcb_ballistics_profile_t profile = s_nominal_profile;
    vcb_ballistics_environment_t environment = s_nominal_wind;
    vcb_ballistics_solution_t solution;

    expect_failure(vcb_ballistics_status_invalid_argument, NULL, &environment,
                   100.0f, "null profile");
    expect_failure(vcb_ballistics_status_invalid_argument, &profile, NULL,
                   100.0f, "null environment");
    check_true(vcb_ballistics_solve(&profile, &environment, 100.0f, NULL) ==
               vcb_ballistics_status_invalid_argument, "null solution");

    float *profile_fields[] = {
        &profile.muzzle_velocity_mps,
        &profile.ballistic_coefficient,
        &profile.sight_height_m,
        &profile.zero_range_m,
    };
    for(size_t i = 0u; i < sizeof(profile_fields) / sizeof(profile_fields[0]); i++){
        const float saved = *profile_fields[i];
        *profile_fields[i] = NAN;
        expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                       100.0f, "profile NaN");
        *profile_fields[i] = INFINITY;
        expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                       100.0f, "profile positive infinity");
        *profile_fields[i] = -INFINITY;
        expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                       100.0f, "profile negative infinity");
        *profile_fields[i] = saved;
    }

    float *environment_fields[] = {
        &environment.wind_speed_mps,
        &environment.wind_direction_deg,
    };
    for(size_t i = 0u; i < sizeof(environment_fields) / sizeof(environment_fields[0]); i++){
        const float saved = *environment_fields[i];
        *environment_fields[i] = NAN;
        expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                       100.0f, "environment NaN");
        *environment_fields[i] = INFINITY;
        expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                       100.0f, "environment positive infinity");
        *environment_fields[i] = -INFINITY;
        expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                       100.0f, "environment negative infinity");
        *environment_fields[i] = saved;
    }

    expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                   NAN, "range NaN");
    expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                   INFINITY, "range positive infinity");
    expect_failure(vcb_ballistics_status_invalid_argument, &profile, &environment,
                   -INFINITY, "range negative infinity");
    poison_solution(&solution);
}

static void test_out_of_range_inputs(void){
    vcb_ballistics_profile_t profile = s_nominal_profile;
    vcb_ballistics_environment_t environment = s_nominal_wind;

    const struct {
        float *field;
        float value;
        const char *name;
    } profile_cases[] = {
        {&profile.muzzle_velocity_mps, 49.99f, "muzzle below minimum"},
        {&profile.muzzle_velocity_mps, 1500.01f, "muzzle above maximum"},
        {&profile.ballistic_coefficient, 0.0499f, "BC below minimum"},
        {&profile.ballistic_coefficient, 2.001f, "BC above maximum"},
        {&profile.sight_height_m, -0.001f, "sight height below minimum"},
        {&profile.sight_height_m, 0.201f, "sight height above maximum"},
        {&profile.zero_range_m, 9.99f, "zero below minimum"},
        {&profile.zero_range_m, 1000.01f, "zero above maximum"},
    };
    for(size_t i = 0u; i < sizeof(profile_cases) / sizeof(profile_cases[0]); i++){
        const float saved = *profile_cases[i].field;
        *profile_cases[i].field = profile_cases[i].value;
        expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                       100.0f, profile_cases[i].name);
        *profile_cases[i].field = saved;
    }

    environment.wind_speed_mps = -0.001f;
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   100.0f, "wind below minimum");
    environment.wind_speed_mps = 100.001f;
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   100.0f, "wind above maximum");
    environment = s_nominal_wind;
    environment.wind_direction_deg = -0.001f;
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   100.0f, "direction below minimum");
    environment.wind_direction_deg = 360.0f;
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   100.0f, "direction upper bound excluded");
    environment = s_nominal_wind;
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   0.999f, "range below minimum");
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   0.0f, "zero range request rejected");
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   -1.0f, "negative range request rejected");
    expect_failure(vcb_ballistics_status_out_of_range, &profile, &environment,
                   2000.01f, "range above maximum");
}

static void check_ok(const vcb_ballistics_profile_t *profile,
                     const vcb_ballistics_environment_t *environment,
                     float range_m,
                     vcb_ballistics_solution_t *solution,
                     const char *name){
    const vcb_ballistics_status_e status =
        vcb_ballistics_solve(profile, environment, range_m, solution);
    check_true(status == vcb_ballistics_status_ok, name);
}

static void test_full_envelope_matrix(void){
    static const struct {
        const char *name;
        vcb_ballistics_profile_t profile;
        unsigned expected_ok;
    } profiles[] = {
        {"low_all",
         {50.0f, 0.05f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1}, 35u},
        {"low_velocity_nominal",
         {50.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, 0u},
        {"nominal_low_bc_long_zero",
         {800.0f, 0.05f, 0.20f, 1000.0f, vcb_ballistics_drag_family_g1}, 0u},
        {"nominal",
         {800.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, 64u},
        {"high_all",
         {1500.0f, 2.0f, 0.20f, 1000.0f, vcb_ballistics_drag_family_g1}, 64u},
        {"high_velocity_low_bc",
         {1500.0f, 0.05f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1}, 48u},
        {"low_velocity_high_bc",
         {50.0f, 2.0f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1}, 58u},
        {"nominal_high_bc_long_zero",
         {800.0f, 2.0f, 0.20f, 1000.0f, vcb_ballistics_drag_family_g1}, 64u},
        {"high_velocity_nominal",
         {1500.0f, 0.5f, 0.05f, 100.0f, vcb_ballistics_drag_family_g1}, 64u},
    };
    const float ranges[] = {1.0f, 100.0f, 1000.0f, 2000.0f};
    const float wind_speeds[] = {0.0f, 100.0f};
    const float directions[] = {
        0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f,
    };
    unsigned total = 0u;
    unsigned total_ok = 0u;
    unsigned total_no_solution = 0u;
    unsigned total_other = 0u;

    for(size_t p = 0u; p < sizeof(profiles) / sizeof(profiles[0]); p++){
        unsigned profile_ok = 0u;
        unsigned profile_no_solution = 0u;
        unsigned profile_other = 0u;
        for(size_t r = 0u; r < sizeof(ranges) / sizeof(ranges[0]); r++){
            for(size_t w = 0u; w < sizeof(wind_speeds) / sizeof(wind_speeds[0]); w++){
                for(size_t d = 0u; d < sizeof(directions) / sizeof(directions[0]); d++){
                    const vcb_ballistics_environment_t environment = {
                        .wind_speed_mps = wind_speeds[w],
                        .wind_direction_deg = directions[d],
                    };
                    vcb_ballistics_solution_t solution;
                    poison_solution(&solution);
                    const vcb_ballistics_status_e status =
                        vcb_ballistics_solve(&profiles[p].profile, &environment,
                                             ranges[r], &solution);
                    total++;
                    if(status == vcb_ballistics_status_ok){
                        profile_ok++;
                        total_ok++;
                        check_true(isfinite(solution.time_of_flight_s) &&
                                   isfinite(solution.impact_velocity_mps) &&
                                   isfinite(solution.drop_m) &&
                                   isfinite(solution.wind_drift_m) &&
                                   isfinite(solution.elevation_correction_mrad) &&
                                   isfinite(solution.windage_correction_mrad),
                                   "matrix finite solution");
                        check_true(solution.time_of_flight_s > 0.0f &&
                                   solution.impact_velocity_mps > 0.0f,
                                   "matrix positive time and velocity");
                        check_true(nearly_equal(solution.elevation_correction_mrad,
                                                atan2f(solution.drop_m, ranges[r]) * 1000.0f,
                                                2.0e-5f, 2.0e-6f),
                                   "matrix elevation conversion");
                        check_true(nearly_equal(solution.windage_correction_mrad,
                                                atan2f(-solution.wind_drift_m,
                                                       ranges[r]) * 1000.0f,
                                                2.0e-5f, 2.0e-6f),
                                   "matrix windage conversion");
                    }else if(status == vcb_ballistics_status_no_solution){
                        profile_no_solution++;
                        total_no_solution++;
                        check_true(solution_is_zero(&solution),
                                   "matrix no-solution clears output");
                    }else{
                        profile_other++;
                        total_other++;
                        check_true(false, "matrix unexpected status");
                        check_true(solution_is_zero(&solution),
                                   "matrix unexpected failure clears output");
                    }
                }
            }
        }
        printf("MATRIX_PROFILE %-28s ok %u no_solution %u other %u\n",
               profiles[p].name, profile_ok, profile_no_solution, profile_other);
        check_true(profile_ok == profiles[p].expected_ok,
                   "matrix profile OK count");
        check_true(profile_no_solution == 64u - profiles[p].expected_ok,
                   "matrix profile no-solution count");
        check_true(profile_other == 0u, "matrix profile has no other status");
    }

    printf("MATRIX_TOTAL cases %u ok %u no_solution %u other %u\n",
           total, total_ok, total_no_solution, total_other);
    check_true(total == 576u, "matrix total case count");
    check_true(total_ok == 397u, "matrix total OK count");
    check_true(total_no_solution == 179u, "matrix total no-solution count");
    check_true(total_other == 0u, "matrix total other count");
}

static void test_zero_and_mirrored_wind(void){
    const float ranges[] = {1.0f, 100.0f, 500.0f, 1000.0f, 2000.0f};
    for(size_t i = 0u; i < sizeof(ranges) / sizeof(ranges[0]); i++){
        vcb_ballistics_environment_t environment = {
            .wind_speed_mps = 0.0f,
            .wind_direction_deg = 0.0f,
        };
        vcb_ballistics_solution_t solution;
        check_ok(&s_nominal_profile, &environment, ranges[i], &solution,
                 "zero wind solve");
        check_true(fabsf(solution.wind_drift_m) <= 1.0e-6f,
                   "zero wind drift");
        check_true(fabsf(solution.windage_correction_mrad) <= 1.0e-6f,
                   "zero wind correction");
    }

    vcb_ballistics_environment_t right_wind = s_nominal_wind;
    vcb_ballistics_environment_t left_wind = s_nominal_wind;
    left_wind.wind_direction_deg = 270.0f;
    vcb_ballistics_solution_t right;
    vcb_ballistics_solution_t left;
    check_ok(&s_nominal_profile, &right_wind, 500.0f, &right,
             "right crosswind solve");
    check_ok(&s_nominal_profile, &left_wind, 500.0f, &left,
             "left crosswind solve");
    check_true(right.wind_drift_m > 0.0f, "90 degree wind drifts right");
    check_true(right.windage_correction_mrad < 0.0f,
               "right drift correction points left");
    check_true(nearly_equal(right.time_of_flight_s, left.time_of_flight_s,
                            1.0e-6f, 1.0e-6f), "mirrored wind time");
    check_true(nearly_equal(right.impact_velocity_mps, left.impact_velocity_mps,
                            1.0e-5f, 1.0e-6f), "mirrored wind velocity");
    check_true(nearly_equal(right.drop_m, left.drop_m, 1.0e-6f, 1.0e-6f),
               "mirrored wind drop");
    check_true(nearly_equal(right.wind_drift_m, -left.wind_drift_m,
                            1.0e-6f, 1.0e-6f), "mirrored wind drift");
    check_true(nearly_equal(right.windage_correction_mrad,
                            -left.windage_correction_mrad,
                            1.0e-6f, 1.0e-6f), "mirrored wind correction");

    const float axial_directions[] = {0.0f, 180.0f};
    for(size_t i = 0u; i < sizeof(axial_directions) / sizeof(axial_directions[0]); i++){
        vcb_ballistics_environment_t environment = s_nominal_wind;
        vcb_ballistics_solution_t solution;
        environment.wind_direction_deg = axial_directions[i];
        check_ok(&s_nominal_profile, &environment, 500.0f, &solution,
                 "axial wind solve");
        check_true(fabsf(solution.wind_drift_m) <= 1.0e-5f,
                   "axial wind drift");
        check_true(fabsf(solution.windage_correction_mrad) <= 1.0e-5f,
                   "axial wind correction");
    }

    const vcb_ballistics_environment_t tail_environment = {5.0f, 0.0f};
    const vcb_ballistics_environment_t head_environment = {5.0f, 180.0f};
    const vcb_ballistics_environment_t right_environment = {5.0f, 90.0f};
    const vcb_ballistics_environment_t oblique_environments[] = {
        {5.0f, 45.0f},
        {5.0f, 315.0f},
        {5.0f, 135.0f},
        {5.0f, 225.0f},
    };
    vcb_ballistics_solution_t tail;
    vcb_ballistics_solution_t head;
    vcb_ballistics_solution_t pure_right;
    vcb_ballistics_solution_t oblique[4];
    check_ok(&s_nominal_profile, &tail_environment, 1000.0f, &tail,
             "tailwind ordering solve");
    check_ok(&s_nominal_profile, &head_environment, 1000.0f, &head,
             "headwind ordering solve");
    check_ok(&s_nominal_profile, &right_environment, 1000.0f, &pure_right,
             "pure crosswind component solve");
    for(size_t i = 0u; i < 4u; i++){
        check_ok(&s_nominal_profile, &oblique_environments[i], 1000.0f,
                 &oblique[i], "oblique wind solve");
    }

    check_true(tail.time_of_flight_s < head.time_of_flight_s,
               "tailwind time below headwind");
    check_true(tail.impact_velocity_mps > head.impact_velocity_mps,
               "tailwind velocity above headwind");
    check_true(tail.drop_m < head.drop_m, "tailwind drop below headwind");
    check_true(oblique[0].time_of_flight_s < pure_right.time_of_flight_s &&
               pure_right.time_of_flight_s < oblique[2].time_of_flight_s,
               "oblique axial time ordering");
    check_true(oblique[0].impact_velocity_mps > pure_right.impact_velocity_mps &&
               pure_right.impact_velocity_mps > oblique[2].impact_velocity_mps,
               "oblique axial velocity ordering");
    check_true(oblique[0].wind_drift_m > 0.0f && oblique[2].wind_drift_m > 0.0f &&
               oblique[1].wind_drift_m < 0.0f && oblique[3].wind_drift_m < 0.0f,
               "oblique lateral signs");
    check_true(fabsf(oblique[0].wind_drift_m) < pure_right.wind_drift_m &&
               fabsf(oblique[2].wind_drift_m) < pure_right.wind_drift_m,
               "oblique lateral components below full value");

    const unsigned mirror_pairs[][2] = {{0u, 1u}, {2u, 3u}};
    for(size_t i = 0u; i < 2u; i++){
        const vcb_ballistics_solution_t *positive = &oblique[mirror_pairs[i][0]];
        const vcb_ballistics_solution_t *negative = &oblique[mirror_pairs[i][1]];
        check_true(nearly_equal(positive->time_of_flight_s, negative->time_of_flight_s,
                                1.0e-6f, 1.0e-6f), "oblique mirror time");
        check_true(nearly_equal(positive->impact_velocity_mps,
                                negative->impact_velocity_mps,
                                1.0e-5f, 1.0e-6f), "oblique mirror velocity");
        check_true(nearly_equal(positive->drop_m, negative->drop_m,
                                1.0e-6f, 1.0e-6f), "oblique mirror drop");
        check_true(nearly_equal(positive->wind_drift_m, -negative->wind_drift_m,
                                1.0e-6f, 1.0e-6f), "oblique mirror drift");
        check_true(nearly_equal(positive->windage_correction_mrad,
                                -negative->windage_correction_mrad,
                                1.0e-6f, 1.0e-6f), "oblique mirror correction");
    }

    printf("WIND_ORDER tail/head tof %.6f/%.6f vel %.3f/%.3f drop %.6f/%.6f "
           "drift45/90/135 %.6f/%.6f/%.6f\n",
           tail.time_of_flight_s, head.time_of_flight_s,
           tail.impact_velocity_mps, head.impact_velocity_mps,
           tail.drop_m, head.drop_m, oblique[0].wind_drift_m,
           pure_right.wind_drift_m, oblique[2].wind_drift_m);
}

static void test_monotonicity_and_consistency(void){
    const float ranges[] = {
        1.0f, 10.0f, 25.0f, 50.0f, 75.0f, 100.0f, 150.0f,
        200.0f, 300.0f, 500.0f, 750.0f, 1000.0f, 1500.0f, 2000.0f,
    };
    vcb_ballistics_solution_t previous = {0};
    for(size_t i = 0u; i < sizeof(ranges) / sizeof(ranges[0]); i++){
        vcb_ballistics_solution_t solution;
        check_ok(&s_nominal_profile, &s_nominal_wind, ranges[i], &solution,
                 "range sweep solve");
        check_true(isfinite(solution.time_of_flight_s) &&
                   solution.time_of_flight_s > 0.0f, "finite positive time");
        check_true(isfinite(solution.impact_velocity_mps) &&
                   solution.impact_velocity_mps > 0.0f &&
                   solution.impact_velocity_mps <= s_nominal_profile.muzzle_velocity_mps,
                   "bounded velocity");
        check_true(isfinite(solution.drop_m) && isfinite(solution.wind_drift_m),
                   "finite linear solution");
        check_true(isfinite(solution.elevation_correction_mrad) &&
                   isfinite(solution.windage_correction_mrad),
                   "finite angular solution");
        printf("SWEEP range %.2f tof %.6f vel %.3f drop %.6f drift %.6f "
               "elev %.6f/%.6f wind %.6f/%.6f\n",
               ranges[i], solution.time_of_flight_s,
               solution.impact_velocity_mps, solution.drop_m,
               solution.wind_drift_m, solution.elevation_correction_mrad,
               atan2f(solution.drop_m, ranges[i]) * 1000.0f,
               solution.windage_correction_mrad,
               atan2f(-solution.wind_drift_m, ranges[i]) * 1000.0f);
        check_true(nearly_equal(solution.elevation_correction_mrad,
                                atan2f(solution.drop_m, ranges[i]) * 1000.0f,
                                2.0e-5f, 2.0e-6f), "elevation conversion");
        check_true(nearly_equal(solution.windage_correction_mrad,
                                atan2f(-solution.wind_drift_m, ranges[i]) * 1000.0f,
                                2.0e-5f, 2.0e-6f), "windage conversion");
        if(i > 0u){
            check_true(solution.time_of_flight_s > previous.time_of_flight_s,
                       "time increases with range");
            check_true(solution.impact_velocity_mps <= previous.impact_velocity_mps,
                       "velocity does not increase");
        }
        if(ranges[i] > s_nominal_profile.zero_range_m &&
           i > 0u && ranges[i - 1u] >= s_nominal_profile.zero_range_m){
            check_true(solution.drop_m >= previous.drop_m, "drop increases past zero");
            check_true(fabsf(solution.elevation_correction_mrad) >=
                       fabsf(previous.elevation_correction_mrad),
                       "elevation correction grows past zero");
            check_true(fabsf(solution.windage_correction_mrad) >=
                       fabsf(previous.windage_correction_mrad),
                       "wind correction grows with range");
        }
        previous = solution;
    }
}

static void test_boundaries_and_repeatability(void){
    static const struct {
        vcb_ballistics_profile_t profile;
        vcb_ballistics_environment_t environment;
        float range_m;
        const char *name;
    } cases[] = {
        {{50.0f, 2.0f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1},
         {0.0f, 0.0f}, 1.0f,
         "minimum velocity solvable boundary"},
        {{1500.0f, 2.0f, 0.20f, 1000.0f, vcb_ballistics_drag_family_g1},
         {100.0f, 359.999f}, 2000.0f,
         "upper solvable boundaries"},
        {{1500.0f, 0.05f, 0.0f, 10.0f, vcb_ballistics_drag_family_g1},
         {100.0f, 90.0f}, 10.0f,
         "minimum BC with maximum wind"},
    };
    for(size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); i++){
        vcb_ballistics_solution_t first;
        vcb_ballistics_solution_t second;
        check_ok(&cases[i].profile, &cases[i].environment, cases[i].range_m,
                 &first, cases[i].name);
        check_ok(&cases[i].profile, &cases[i].environment, cases[i].range_m,
                 &second, "repeat boundary solve");
        check_true(memcmp(&first, &second, sizeof(first)) == 0,
                   "repeat solve is bitwise stable");
    }

    const vcb_ballistics_profile_t impossible_zero = {
        .muzzle_velocity_mps = 50.0f,
        .ballistic_coefficient = 0.05f,
        .sight_height_m = 0.20f,
        .zero_range_m = 1000.0f,
        .drag_family = vcb_ballistics_drag_family_g1,
    };
    expect_failure(vcb_ballistics_status_no_solution, &impossible_zero,
                   &s_nominal_wind, 100.0f, "impossible low-angle zero");
}

/* Separation floors frozen 2026-08-18 from the two captured source tables;
   see the G7 section of docs/ballistics-validation-reference.md. */
static void test_drag_family_discrimination(void){
    static const struct {
        float range_m;
        float min_drop_separation_m; /* 0 means direction-only at this range */
        float min_velocity_separation_mps;
        float min_time_separation_s;
        float min_drift_separation_m;
    } cases[] = {
        {274.32f, 0.0f, 0.0f, 0.0f, 0.0f},
        {457.20f, 0.0f, 80.0f, 0.0f, 0.0f},
        {640.08f, 1.0f, 0.0f, 0.0f, 0.0f},
        {914.40f, 5.0f, 0.0f, 0.30f, 1.5f},
    };
    vcb_ballistics_profile_t g1_profile = s_g7_profile;
    g1_profile.drag_family = vcb_ballistics_drag_family_g1;

    for(size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); i++){
        vcb_ballistics_solution_t g7;
        vcb_ballistics_solution_t g1;
        check_ok(&s_g7_profile, &s_nominal_wind, cases[i].range_m, &g7,
                 "family discrimination G7 solve");
        check_ok(&g1_profile, &s_nominal_wind, cases[i].range_m, &g1,
                 "family discrimination G1 solve");

        const float drop_separation = g1.drop_m - g7.drop_m;
        const float velocity_separation = g7.impact_velocity_mps - g1.impact_velocity_mps;
        const float time_separation = g1.time_of_flight_s - g7.time_of_flight_s;
        const float drift_separation = g1.wind_drift_m - g7.wind_drift_m;

        printf("FAMILY %7.2f m drop %.6f/%.6f sep %.6f vel %.3f/%.3f sep %.3f "
               "tof %.6f/%.6f sep %.6f drift %.6f/%.6f sep %.6f\n",
               cases[i].range_m, g7.drop_m, g1.drop_m, drop_separation,
               g7.impact_velocity_mps, g1.impact_velocity_mps, velocity_separation,
               g7.time_of_flight_s, g1.time_of_flight_s, time_separation,
               g7.wind_drift_m, g1.wind_drift_m, drift_separation);

        /* Direction: for one BC the G7 standard is the lower-drag reference,
           so G1 must drop more, fly longer, drift more, and retain less speed.
           A selector wired backwards passes magnitudes and fails this. */
        check_true(drop_separation > 0.0f, "G1 drops more than G7 at equal BC");
        check_true(time_separation > 0.0f, "G1 flies longer than G7 at equal BC");
        check_true(drift_separation > 0.0f, "G1 drifts more than G7 at equal BC");
        check_true(velocity_separation > 0.0f, "G7 retains more speed than G1");
        check_true(g1.elevation_correction_mrad > g7.elevation_correction_mrad,
                   "G1 needs more elevation than G7 at equal BC");

        if(cases[i].min_drop_separation_m > 0.0f){
            check_true(drop_separation >= cases[i].min_drop_separation_m,
                       "family drop separation floor");
        }
        if(cases[i].min_velocity_separation_mps > 0.0f){
            check_true(velocity_separation >= cases[i].min_velocity_separation_mps,
                       "family velocity separation floor");
        }
        if(cases[i].min_time_separation_s > 0.0f){
            check_true(time_separation >= cases[i].min_time_separation_s,
                       "family time separation floor");
        }
        if(cases[i].min_drift_separation_m > 0.0f){
            check_true(drift_separation >= cases[i].min_drift_separation_m,
                       "family drift separation floor");
        }
    }

    /* G7 must be a distinct drag curve, not G1 with a rescaled BC. The G7
       BC 0.250 and G1 BC 0.500 fixtures agree near the muzzle and must part
       company once velocity decays. */
    vcb_ballistics_solution_t g7_near;
    vcb_ballistics_solution_t g1_near;
    vcb_ballistics_solution_t g7_far;
    vcb_ballistics_solution_t g1_far;
    check_ok(&s_g7_profile, &s_nominal_wind, 182.88f, &g7_near, "rescale near G7 solve");
    check_ok(&s_nominal_profile, &s_nominal_wind, 182.88f, &g1_near, "rescale near G1 solve");
    check_ok(&s_g7_profile, &s_nominal_wind, 914.40f, &g7_far, "rescale far G7 solve");
    check_ok(&s_nominal_profile, &s_nominal_wind, 914.40f, &g1_far, "rescale far G1 solve");
    const float near_drop_delta = fabsf(g7_near.drop_m - g1_near.drop_m);
    const float far_velocity_delta =
        fabsf(g7_far.impact_velocity_mps - g1_far.impact_velocity_mps);
    printf("RESCALE near drop delta %.6f far velocity delta %.3f\n",
           near_drop_delta, far_velocity_delta);
    check_true(near_drop_delta <= 0.05f, "G7 0.25 tracks G1 0.50 near the muzzle");
    check_true(far_velocity_delta >= 10.0f, "G7 0.25 parts from G1 0.50 downrange");
}

static void test_invalid_drag_family(void){
    const int g1_value = (int)vcb_ballistics_drag_family_g1;
    const int g7_value = (int)vcb_ballistics_drag_family_g7;
    check_true(g1_value == 0, "G1 family constant is zero");
    check_true(g7_value == 1, "G7 family constant is one");

    static const int invalid_families[] = {2, 255};
    for(size_t i = 0u; i < sizeof(invalid_families) / sizeof(invalid_families[0]); i++){
        vcb_ballistics_profile_t profile = s_nominal_profile;
        profile.drag_family = (vcb_ballistics_drag_family_e)invalid_families[i];
        expect_failure(vcb_ballistics_status_invalid_argument, &profile,
                       &s_nominal_wind, 100.0f, "invalid drag family rejected");
    }

    /* The BC bounds are family independent. */
    vcb_ballistics_profile_t g7_profile = s_g7_profile;
    g7_profile.ballistic_coefficient = 0.0499f;
    expect_failure(vcb_ballistics_status_out_of_range, &g7_profile, &s_nominal_wind,
                   100.0f, "G7 BC below minimum");
    g7_profile.ballistic_coefficient = 2.001f;
    expect_failure(vcb_ballistics_status_out_of_range, &g7_profile, &s_nominal_wind,
                   100.0f, "G7 BC above maximum");
    const float inclusive_bounds[] = {VCB_BALLISTICS_MIN_BC, VCB_BALLISTICS_MAX_BC};
    for(size_t i = 0u; i < sizeof(inclusive_bounds) / sizeof(inclusive_bounds[0]); i++){
        vcb_ballistics_solution_t solution;
        g7_profile.ballistic_coefficient = inclusive_bounds[i];
        check_ok(&g7_profile, &s_nominal_wind, 100.0f, &solution,
                 "G7 BC inclusive boundary accepted");
    }
}

static void test_zero_initialised_family_is_g1(void){
    vcb_ballistics_profile_t implicit_profile;
    memset(&implicit_profile, 0, sizeof(implicit_profile));
    implicit_profile.muzzle_velocity_mps = 800.0f;
    implicit_profile.ballistic_coefficient = 0.5f;
    implicit_profile.sight_height_m = 0.05f;
    implicit_profile.zero_range_m = 100.0f;
    /* drag_family deliberately left at its zero-initialised value. */

    vcb_ballistics_profile_t explicit_g1 = implicit_profile;
    explicit_g1.drag_family = vcb_ballistics_drag_family_g1;

    const float ranges[] = {1.0f, 100.0f, 500.0f, 1000.0f, 2000.0f};
    for(size_t i = 0u; i < sizeof(ranges) / sizeof(ranges[0]); i++){
        vcb_ballistics_solution_t implicit_solution;
        vcb_ballistics_solution_t explicit_solution;
        poison_solution(&implicit_solution);
        poison_solution(&explicit_solution);
        const vcb_ballistics_status_e implicit_status =
            vcb_ballistics_solve(&implicit_profile, &s_nominal_wind, ranges[i],
                                 &implicit_solution);
        const vcb_ballistics_status_e explicit_status =
            vcb_ballistics_solve(&explicit_g1, &s_nominal_wind, ranges[i],
                                 &explicit_solution);
        check_true(implicit_status == explicit_status,
                   "zero-initialised family matches explicit G1 status");
        check_true(memcmp(&implicit_solution, &explicit_solution,
                          sizeof(implicit_solution)) == 0,
                   "zero-initialised family matches explicit G1 output");
    }
}

int main(int argc, char **argv){
    if(argc != 3){
        fprintf(stderr, "usage: %s reference_cases.csv reference_cases_g7.csv\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    test_reference_cases(argv[1], 22u);
    test_reference_cases(argv[2], 10u);
    test_null_and_nonfinite_inputs();
    test_out_of_range_inputs();
    test_full_envelope_matrix();
    test_zero_and_mirrored_wind();
    test_monotonicity_and_consistency();
    test_boundaries_and_repeatability();
    test_drag_family_discrimination();
    test_invalid_drag_family();
    test_zero_initialised_family_is_g1();

    printf("SUMMARY checks=%u failures=%u\n", s_checks, s_failures);
    return s_failures == 0u ? EXIT_SUCCESS : EXIT_FAILURE;
}
