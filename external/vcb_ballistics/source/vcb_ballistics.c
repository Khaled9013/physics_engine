#include "vcb_ballistics.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define VCB_BALLISTICS_GRAVITY_MPS2 9.80665f
#define VCB_BALLISTICS_FPS_TO_MPS 0.3048f
#define VCB_BALLISTICS_DEG_TO_RAD 0.01745329251994329577f
#define VCB_BALLISTICS_MAX_ZERO_ANGLE_RAD 0.08726646259971647884f
#define VCB_BALLISTICS_ZERO_ITERATIONS 24u
#define VCB_BALLISTICS_MIN_FORWARD_VELOCITY_MPS 1.0f

#ifndef VCB_BALLISTICS_STEP_M
#define VCB_BALLISTICS_STEP_M 0.5f
#endif

typedef struct {
    float min_velocity_fps;
    float coefficient;
    float exponent;
} vcb_ballistics_drag_segment_t;

typedef struct {
    float y;
    float z;
    float vx;
    float vy;
    float vz;
    float time;
} vcb_ballistics_state_t;

typedef struct {
    float y;
    float z;
    float vx;
    float vy;
    float vz;
    float time;
} vcb_ballistics_derivative_t;

static const vcb_ballistics_drag_segment_t s_g1_segments[] = {
    {4230.0f, 1.477404177730177e-04f, 1.9565f},
    {3680.0f, 1.920339268755614e-04f, 1.925f},
    {3450.0f, 2.894751026819746e-04f, 1.875f},
    {3295.0f, 4.349905111115636e-04f, 1.825f},
    {3130.0f, 6.520421871892662e-04f, 1.775f},
    {2960.0f, 9.748073694078696e-04f, 1.725f},
    {2830.0f, 1.453721560187286e-03f, 1.675f},
    {2680.0f, 2.162887202930376e-03f, 1.625f},
    {2460.0f, 3.209559783129881e-03f, 1.575f},
    {2225.0f, 3.904368218691249e-03f, 1.55f},
    {2015.0f, 3.222942271262336e-03f, 1.575f},
    {1890.0f, 2.203329542297809e-03f, 1.625f},
    {1810.0f, 1.511001028891904e-03f, 1.675f},
    {1730.0f, 8.609957592468259e-04f, 1.75f},
    {1595.0f, 4.086146797305117e-04f, 1.85f},
    {1520.0f, 1.954473210037398e-04f, 1.95f},
    {1420.0f, 5.431896266462351e-05f, 2.125f},
    {1360.0f, 8.847742581674416e-06f, 2.375f},
    {1315.0f, 1.456922328720298e-06f, 2.625f},
    {1280.0f, 2.419485191895565e-07f, 2.875f},
    {1220.0f, 1.657956321067612e-08f, 3.25f},
    {1185.0f, 4.745469537157371e-10f, 3.75f},
    {1150.0f, 1.379746590025088e-11f, 4.25f},
    {1100.0f, 4.070157961147882e-13f, 4.75f},
    {1060.0f, 2.938236954847331e-14f, 5.125f},
    {1025.0f, 1.228597370774746e-14f, 5.25f},
    {980.0f, 2.916938264100495e-14f, 5.125f},
    {945.0f, 3.855099424807451e-13f, 4.75f},
    {905.0f, 1.185097045689854e-11f, 4.25f},
    {860.0f, 3.566129470974951e-10f, 3.75f},
    {810.0f, 1.045513263966272e-08f, 3.25f},
    {780.0f, 1.291159200846216e-07f, 2.875f},
    {750.0f, 6.824429329105383e-07f, 2.625f},
    {700.0f, 3.569169672385163e-06f, 2.375f},
    {640.0f, 1.839015095899579e-05f, 2.125f},
    {600.0f, 5.711174688734240e-05f, 1.95f},
    {550.0f, 9.226557091973427e-05f, 1.875f},
    {250.0f, 9.337991957131389e-05f, 1.875f},
    {100.0f, 7.225247327590413e-05f, 1.925f},
    {65.0f, 5.792684957074546e-05f, 1.975f},
    {0.0f, 5.206214107320588e-05f, 2.0f}
};

static const vcb_ballistics_drag_segment_t s_g7_segments[] = {
    {4200.0f, 1.29081656775919e-09f, 3.24121295355962f},
    {3000.0f, 0.0171422231434847f, 1.27907168025204f},
    {1470.0f, 2.33355948302505e-03f, 1.52693913274526f},
    {1260.0f, 7.97592111627665e-04f, 1.67688974440324f},
    {1110.0f, 5.71086414289273e-12f, 4.3212826264889f},
    {960.0f, 3.02865108244904e-17f, 5.99074203776707f},
    {670.0f, 7.52285155782535e-06f, 2.1738019851075f},
    {540.0f, 1.31766281225189e-05f, 2.08774690257991f},
    {0.0f, 1.34504843776525e-05f, 2.08702306738884f}
};

static int vcb_ballistics_float_valid(float value)
{
    return isfinite(value) != 0;
}

static int vcb_ballistics_value_in_range(float value, float minimum, float maximum)
{
    return value >= minimum && value <= maximum;
}

static int vcb_ballistics_state_valid(const vcb_ballistics_state_t *state)
{
    return vcb_ballistics_float_valid(state->y) &&
           vcb_ballistics_float_valid(state->z) &&
           vcb_ballistics_float_valid(state->vx) &&
           vcb_ballistics_float_valid(state->vy) &&
           vcb_ballistics_float_valid(state->vz) &&
           vcb_ballistics_float_valid(state->time);
}

static float vcb_ballistics_segment_drag_mps2(
        const vcb_ballistics_drag_segment_t *segments,
        size_t segment_count,
        float velocity_mps,
        float ballistic_coefficient)
{
    const float velocity_fps = velocity_mps / VCB_BALLISTICS_FPS_TO_MPS;
    size_t i;

    if(velocity_fps <= 0.0f){
        return 0.0f;
    }

    for(i = 0u; i < segment_count; i++){
        if(velocity_fps > segments[i].min_velocity_fps){
            const float drag_fps2 = segments[i].coefficient *
                    powf(velocity_fps, segments[i].exponent) /
                    ballistic_coefficient;
            return drag_fps2 * VCB_BALLISTICS_FPS_TO_MPS;
        }
    }

    return 0.0f;
}

static float vcb_ballistics_g1_drag_mps2(float velocity_mps, float ballistic_coefficient)
{
    return vcb_ballistics_segment_drag_mps2(
            s_g1_segments, sizeof(s_g1_segments) / sizeof(s_g1_segments[0]),
            velocity_mps, ballistic_coefficient);
}

static float vcb_ballistics_g7_drag_mps2(float velocity_mps, float ballistic_coefficient)
{
    return vcb_ballistics_segment_drag_mps2(
            s_g7_segments, sizeof(s_g7_segments) / sizeof(s_g7_segments[0]),
            velocity_mps, ballistic_coefficient);
}

static float vcb_ballistics_drag_mps2(
        vcb_ballistics_drag_family_e drag_family,
        float velocity_mps,
        float ballistic_coefficient)
{
    if(drag_family == vcb_ballistics_drag_family_g7){
        return vcb_ballistics_g7_drag_mps2(velocity_mps, ballistic_coefficient);
    }
    return vcb_ballistics_g1_drag_mps2(velocity_mps, ballistic_coefficient);
}

static vcb_ballistics_status_e vcb_ballistics_derivative(
        const vcb_ballistics_state_t *state,
        float wind_vx,
        float wind_vz,
        float ballistic_coefficient,
        vcb_ballistics_drag_family_e drag_family,
        vcb_ballistics_derivative_t *derivative)
{
    const float relative_vx = state->vx - wind_vx;
    const float relative_vy = state->vy;
    const float relative_vz = state->vz - wind_vz;
    const float relative_speed_sq = relative_vx * relative_vx +
            relative_vy * relative_vy + relative_vz * relative_vz;
    float ax = 0.0f;
    float ay = -VCB_BALLISTICS_GRAVITY_MPS2;
    float az = 0.0f;

    if(!vcb_ballistics_state_valid(state) ||
       !vcb_ballistics_float_valid(relative_speed_sq)){
        return vcb_ballistics_status_numeric_error;
    }
    if(state->vx <= VCB_BALLISTICS_MIN_FORWARD_VELOCITY_MPS){
        return vcb_ballistics_status_no_solution;
    }
    if(relative_speed_sq > 0.0f){
        const float relative_speed = sqrtf(relative_speed_sq);
        const float drag = vcb_ballistics_drag_mps2(drag_family, relative_speed,
                                                    ballistic_coefficient);
        const float drag_scale = -drag / relative_speed;

        if(!vcb_ballistics_float_valid(drag_scale)){
            return vcb_ballistics_status_numeric_error;
        }
        ax = drag_scale * relative_vx;
        ay += drag_scale * relative_vy;
        az = drag_scale * relative_vz;
    }

    derivative->y = state->vy / state->vx;
    derivative->z = state->vz / state->vx;
    derivative->vx = ax / state->vx;
    derivative->vy = ay / state->vx;
    derivative->vz = az / state->vx;
    derivative->time = 1.0f / state->vx;

    if(!vcb_ballistics_float_valid(derivative->y) ||
       !vcb_ballistics_float_valid(derivative->z) ||
       !vcb_ballistics_float_valid(derivative->vx) ||
       !vcb_ballistics_float_valid(derivative->vy) ||
       !vcb_ballistics_float_valid(derivative->vz) ||
       !vcb_ballistics_float_valid(derivative->time)){
        return vcb_ballistics_status_numeric_error;
    }
    return vcb_ballistics_status_ok;
}

static vcb_ballistics_state_t vcb_ballistics_state_offset(
        const vcb_ballistics_state_t *state,
        const vcb_ballistics_derivative_t *derivative,
        float distance)
{
    vcb_ballistics_state_t result;

    result.y = state->y + derivative->y * distance;
    result.z = state->z + derivative->z * distance;
    result.vx = state->vx + derivative->vx * distance;
    result.vy = state->vy + derivative->vy * distance;
    result.vz = state->vz + derivative->vz * distance;
    result.time = state->time + derivative->time * distance;
    return result;
}

static vcb_ballistics_status_e vcb_ballistics_integrate_step(
        vcb_ballistics_state_t *state,
        float distance,
        float wind_vx,
        float wind_vz,
        float ballistic_coefficient,
        vcb_ballistics_drag_family_e drag_family)
{
    vcb_ballistics_derivative_t k1;
    vcb_ballistics_derivative_t k2;
    vcb_ballistics_derivative_t k3;
    vcb_ballistics_derivative_t k4;
    vcb_ballistics_state_t intermediate;
    vcb_ballistics_status_e status;

    status = vcb_ballistics_derivative(state, wind_vx, wind_vz,
                                       ballistic_coefficient, drag_family, &k1);
    if(status != vcb_ballistics_status_ok){
        return status;
    }
    intermediate = vcb_ballistics_state_offset(state, &k1, distance * 0.5f);
    status = vcb_ballistics_derivative(&intermediate, wind_vx, wind_vz,
                                       ballistic_coefficient, drag_family, &k2);
    if(status != vcb_ballistics_status_ok){
        return status;
    }
    intermediate = vcb_ballistics_state_offset(state, &k2, distance * 0.5f);
    status = vcb_ballistics_derivative(&intermediate, wind_vx, wind_vz,
                                       ballistic_coefficient, drag_family, &k3);
    if(status != vcb_ballistics_status_ok){
        return status;
    }
    intermediate = vcb_ballistics_state_offset(state, &k3, distance);
    status = vcb_ballistics_derivative(&intermediate, wind_vx, wind_vz,
                                       ballistic_coefficient, drag_family, &k4);
    if(status != vcb_ballistics_status_ok){
        return status;
    }

    state->y += distance * (k1.y + 2.0f * k2.y + 2.0f * k3.y + k4.y) / 6.0f;
    state->z += distance * (k1.z + 2.0f * k2.z + 2.0f * k3.z + k4.z) / 6.0f;
    state->vx += distance * (k1.vx + 2.0f * k2.vx + 2.0f * k3.vx + k4.vx) / 6.0f;
    state->vy += distance * (k1.vy + 2.0f * k2.vy + 2.0f * k3.vy + k4.vy) / 6.0f;
    state->vz += distance * (k1.vz + 2.0f * k2.vz + 2.0f * k3.vz + k4.vz) / 6.0f;
    state->time += distance * (k1.time + 2.0f * k2.time + 2.0f * k3.time + k4.time) / 6.0f;

    if(!vcb_ballistics_state_valid(state)){
        return vcb_ballistics_status_numeric_error;
    }
    if(state->vx <= VCB_BALLISTICS_MIN_FORWARD_VELOCITY_MPS){
        return vcb_ballistics_status_no_solution;
    }
    return vcb_ballistics_status_ok;
}

static vcb_ballistics_status_e vcb_ballistics_integrate(
        const vcb_ballistics_profile_t *profile,
        float launch_angle_rad,
        float wind_vx,
        float wind_vz,
        float range_m,
        vcb_ballistics_state_t *state)
{
    const unsigned int max_steps = (unsigned int)(range_m / VCB_BALLISTICS_STEP_M) + 2u;
    float distance_m = 0.0f;
    unsigned int step;

    state->y = -profile->sight_height_m;
    state->z = 0.0f;
    state->vx = profile->muzzle_velocity_mps * cosf(launch_angle_rad);
    state->vy = profile->muzzle_velocity_mps * sinf(launch_angle_rad);
    state->vz = 0.0f;
    state->time = 0.0f;

    for(step = 0u; distance_m < range_m && step < max_steps; step++){
        const float remaining_m = range_m - distance_m;
        const float step_m = remaining_m < VCB_BALLISTICS_STEP_M ?
                remaining_m : VCB_BALLISTICS_STEP_M;
        const vcb_ballistics_status_e status = vcb_ballistics_integrate_step(
                state, step_m, wind_vx, wind_vz,
                profile->ballistic_coefficient, profile->drag_family);

        if(status != vcb_ballistics_status_ok){
            return status;
        }
        distance_m += step_m;
    }

    if(distance_m < range_m){
        return vcb_ballistics_status_numeric_error;
    }
    return vcb_ballistics_status_ok;
}

static vcb_ballistics_status_e vcb_ballistics_find_zero_angle(
        const vcb_ballistics_profile_t *profile,
        float *zero_angle_rad)
{
    vcb_ballistics_state_t state;
    vcb_ballistics_status_e status;
    float low_angle = 0.0f;
    float high_angle = VCB_BALLISTICS_MAX_ZERO_ANGLE_RAD;
    unsigned int i;

    status = vcb_ballistics_integrate(profile, low_angle, 0.0f, 0.0f,
                                      profile->zero_range_m, &state);
    if(status != vcb_ballistics_status_ok){
        return status;
    }
    if(state.y > 0.0f){
        return vcb_ballistics_status_no_solution;
    }

    status = vcb_ballistics_integrate(profile, high_angle, 0.0f, 0.0f,
                                      profile->zero_range_m, &state);
    if(status != vcb_ballistics_status_ok){
        return status;
    }
    if(state.y < 0.0f){
        return vcb_ballistics_status_no_solution;
    }

    for(i = 0u; i < VCB_BALLISTICS_ZERO_ITERATIONS; i++){
        const float middle_angle = (low_angle + high_angle) * 0.5f;

        status = vcb_ballistics_integrate(profile, middle_angle, 0.0f, 0.0f,
                                          profile->zero_range_m, &state);
        if(status != vcb_ballistics_status_ok){
            return status;
        }
        if(state.y < 0.0f){
            low_angle = middle_angle;
        } else {
            high_angle = middle_angle;
        }
    }

    *zero_angle_rad = (low_angle + high_angle) * 0.5f;
    return vcb_ballistics_status_ok;
}

static vcb_ballistics_status_e vcb_ballistics_validate(
        const vcb_ballistics_profile_t *profile,
        const vcb_ballistics_environment_t *environment,
        float range_m)
{
    if(!vcb_ballistics_float_valid(profile->muzzle_velocity_mps) ||
       !vcb_ballistics_float_valid(profile->ballistic_coefficient) ||
       !vcb_ballistics_float_valid(profile->sight_height_m) ||
       !vcb_ballistics_float_valid(profile->zero_range_m) ||
       !vcb_ballistics_float_valid(environment->wind_speed_mps) ||
       !vcb_ballistics_float_valid(environment->wind_direction_deg) ||
       !vcb_ballistics_float_valid(range_m)){
        return vcb_ballistics_status_invalid_argument;
    }

    /* Any family outside the enumerators is a caller defect, not a range error. */
    if(profile->drag_family != vcb_ballistics_drag_family_g1 &&
       profile->drag_family != vcb_ballistics_drag_family_g7){
        return vcb_ballistics_status_invalid_argument;
    }

    if(!vcb_ballistics_value_in_range(profile->muzzle_velocity_mps,
                                      VCB_BALLISTICS_MIN_MUZZLE_VELOCITY_MPS,
                                      VCB_BALLISTICS_MAX_MUZZLE_VELOCITY_MPS) ||
       !vcb_ballistics_value_in_range(profile->ballistic_coefficient,
                                      VCB_BALLISTICS_MIN_BC,
                                      VCB_BALLISTICS_MAX_BC) ||
       !vcb_ballistics_value_in_range(profile->sight_height_m,
                                      VCB_BALLISTICS_MIN_SIGHT_HEIGHT_M,
                                      VCB_BALLISTICS_MAX_SIGHT_HEIGHT_M) ||
       !vcb_ballistics_value_in_range(profile->zero_range_m,
                                      VCB_BALLISTICS_MIN_ZERO_RANGE_M,
                                      VCB_BALLISTICS_MAX_ZERO_RANGE_M) ||
       !vcb_ballistics_value_in_range(range_m,
                                      VCB_BALLISTICS_MIN_RANGE_M,
                                      VCB_BALLISTICS_MAX_RANGE_M) ||
       !vcb_ballistics_value_in_range(environment->wind_speed_mps,
                                      VCB_BALLISTICS_MIN_WIND_SPEED_MPS,
                                      VCB_BALLISTICS_MAX_WIND_SPEED_MPS) ||
       environment->wind_direction_deg < 0.0f ||
       environment->wind_direction_deg >= 360.0f){
        return vcb_ballistics_status_out_of_range;
    }

    return vcb_ballistics_status_ok;
}

vcb_ballistics_status_e vcb_ballistics_solve(
        const vcb_ballistics_profile_t *profile,
        const vcb_ballistics_environment_t *environment,
        float range_m,
        vcb_ballistics_solution_t *solution)
{
    vcb_ballistics_state_t state;
    vcb_ballistics_status_e status;
    float zero_angle_rad;
    float wind_direction_rad;
    float wind_vx;
    float wind_vz;

    if(solution == NULL){
        return vcb_ballistics_status_invalid_argument;
    }
    memset(solution, 0, sizeof(*solution));
    if(profile == NULL || environment == NULL){
        return vcb_ballistics_status_invalid_argument;
    }

    status = vcb_ballistics_validate(profile, environment, range_m);
    if(status != vcb_ballistics_status_ok){
        return status;
    }
    status = vcb_ballistics_find_zero_angle(profile, &zero_angle_rad);
    if(status != vcb_ballistics_status_ok){
        return status;
    }

    wind_direction_rad = environment->wind_direction_deg * VCB_BALLISTICS_DEG_TO_RAD;
    wind_vx = environment->wind_speed_mps * cosf(wind_direction_rad);
    wind_vz = environment->wind_speed_mps * sinf(wind_direction_rad);
    status = vcb_ballistics_integrate(profile, zero_angle_rad, wind_vx, wind_vz,
                                      range_m, &state);
    if(status != vcb_ballistics_status_ok){
        return status;
    }

    solution->time_of_flight_s = state.time;
    solution->impact_velocity_mps = sqrtf(state.vx * state.vx +
                                           state.vy * state.vy +
                                           state.vz * state.vz);
    solution->drop_m = -state.y;
    solution->wind_drift_m = state.z;
    solution->elevation_correction_mrad = atan2f(solution->drop_m, range_m) * 1000.0f;
    solution->windage_correction_mrad = atan2f(-solution->wind_drift_m, range_m) * 1000.0f;

    if(!vcb_ballistics_float_valid(solution->time_of_flight_s) ||
       !vcb_ballistics_float_valid(solution->impact_velocity_mps) ||
       !vcb_ballistics_float_valid(solution->drop_m) ||
       !vcb_ballistics_float_valid(solution->wind_drift_m) ||
       !vcb_ballistics_float_valid(solution->elevation_correction_mrad) ||
       !vcb_ballistics_float_valid(solution->windage_correction_mrad)){
        memset(solution, 0, sizeof(*solution));
        return vcb_ballistics_status_numeric_error;
    }

    return vcb_ballistics_status_ok;
}
