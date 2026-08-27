#ifndef VCB_BALLISTICS_H_
#define VCB_BALLISTICS_H_

#define VCB_BALLISTICS_MIN_MUZZLE_VELOCITY_MPS 50.0f
#define VCB_BALLISTICS_MAX_MUZZLE_VELOCITY_MPS 1500.0f
#define VCB_BALLISTICS_MIN_BC 0.05f
#define VCB_BALLISTICS_MAX_BC 2.0f
#define VCB_BALLISTICS_MIN_SIGHT_HEIGHT_M 0.0f
#define VCB_BALLISTICS_MAX_SIGHT_HEIGHT_M 0.20f
#define VCB_BALLISTICS_MIN_ZERO_RANGE_M 10.0f
#define VCB_BALLISTICS_MAX_ZERO_RANGE_M 1000.0f
#define VCB_BALLISTICS_MIN_RANGE_M 1.0f
#define VCB_BALLISTICS_MAX_RANGE_M 2000.0f
#define VCB_BALLISTICS_MIN_WIND_SPEED_MPS 0.0f
#define VCB_BALLISTICS_MAX_WIND_SPEED_MPS 100.0f

typedef enum {
    vcb_ballistics_status_ok = 0,
    vcb_ballistics_status_invalid_argument = -1,
    vcb_ballistics_status_out_of_range = -2,
    vcb_ballistics_status_no_solution = -3,
    vcb_ballistics_status_numeric_error = -4
} vcb_ballistics_status_e;

typedef enum {
    vcb_ballistics_drag_family_g1 = 0,
    vcb_ballistics_drag_family_g7 = 1
} vcb_ballistics_drag_family_e;

typedef struct {
    float muzzle_velocity_mps;
    float ballistic_coefficient; /* Conventional BC in the family selected by drag_family, standard atmosphere. */
    float sight_height_m;
    float zero_range_m; /* Calm zero on the line-of-sight range axis. */
    vcb_ballistics_drag_family_e drag_family; /* Selects the retardation table only; zero means G1. */
} vcb_ballistics_profile_t;

typedef struct {
    float wind_speed_mps;
    float wind_direction_deg; /* Travel-to degrees clockwise from the target line; 90 is right. */
} vcb_ballistics_environment_t;

typedef struct {
    float time_of_flight_s;
    float impact_velocity_mps; /* Ground-relative speed at the requested range. */
    float drop_m; /* Positive is below the line of sight. */
    float wind_drift_m; /* Positive is right of the line of sight. */
    float elevation_correction_mrad; /* Positive correction aims up. */
    float windage_correction_mrad; /* Positive correction aims right. */
} vcb_ballistics_solution_t;

vcb_ballistics_status_e vcb_ballistics_solve(
        const vcb_ballistics_profile_t *profile,
        const vcb_ballistics_environment_t *environment,
        float range_m, /* Distance on the line-of-sight range axis. */
        vcb_ballistics_solution_t *solution);

#endif /* VCB_BALLISTICS_H_ */
