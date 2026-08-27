#include <float.h>
#include <math.h>
#include <stdio.h>

#include "../../source/vcb_ballistics.c"

static unsigned int s_checks;
static unsigned int s_failures;

static void check_true(int condition, const char *name, size_t index)
{
    s_checks++;
    if(!condition){
        s_failures++;
        printf("FAIL threshold[%zu] %s\n", index, name);
    }
}

static float segment_drag(size_t index, float velocity_mps)
{
    const float velocity_fps = velocity_mps / VCB_BALLISTICS_FPS_TO_MPS;

    return s_g7_segments[index].coefficient *
            powf(velocity_fps, s_g7_segments[index].exponent) *
            VCB_BALLISTICS_FPS_TO_MPS;
}

static float relative_delta(float actual, float expected)
{
    return fabsf(actual - expected) / fmaxf(fabsf(expected), FLT_MIN);
}

int main(void)
{
    const size_t segment_count =
            sizeof(s_g7_segments) / sizeof(s_g7_segments[0]);
    float worst_boundary_jump = 0.0f;
    size_t worst_boundary_index = 0u;
    size_t exact_count = 0u;
    size_t i;

    check_true(segment_count == 9u, "segment count is nine", 0u);
    check_true(vcb_ballistics_g7_drag_mps2(0.0f, 1.0f) == 0.0f,
               "zero velocity", 0u);
    check_true(vcb_ballistics_g7_drag_mps2(-1.0f, 1.0f) == 0.0f,
               "negative velocity", 0u);
    /* the dispatcher must route G7 and must treat any non-G7 value as G1 */
    check_true(vcb_ballistics_drag_mps2(vcb_ballistics_drag_family_g7, 400.0f,
                                        1.0f) ==
               vcb_ballistics_g7_drag_mps2(400.0f, 1.0f),
               "dispatcher routes G7", 0u);
    check_true(vcb_ballistics_drag_mps2(vcb_ballistics_drag_family_g1, 400.0f,
                                        1.0f) ==
               vcb_ballistics_g1_drag_mps2(400.0f, 1.0f),
               "dispatcher routes G1", 0u);

    printf("G7 knot discontinuities (upper segment vs lower segment at knot):\n");
    for(i = 0u; i + 1u < segment_count; i++){
        const float threshold_fps = s_g7_segments[i].min_velocity_fps;
        const float threshold_mps = threshold_fps * VCB_BALLISTICS_FPS_TO_MPS;
        float below_mps = threshold_mps;
        float above_mps;
        unsigned int search;

        /* descending order is what makes the linear scan correct */
        check_true(s_g7_segments[i].min_velocity_fps >
                   s_g7_segments[i + 1u].min_velocity_fps,
                   "strictly descending thresholds", i);

        for(search = 0u; search < 64u &&
             below_mps / VCB_BALLISTICS_FPS_TO_MPS > threshold_fps; search++){
            below_mps = nextafterf(below_mps, -INFINITY);
        }
        above_mps = nextafterf(below_mps, INFINITY);
        for(search = 0u; search < 64u &&
             above_mps / VCB_BALLISTICS_FPS_TO_MPS <= threshold_fps; search++){
            below_mps = above_mps;
            above_mps = nextafterf(above_mps, INFINITY);
        }

        {
            const float threshold_drag = vcb_ballistics_g7_drag_mps2(
                    threshold_mps, 1.0f);
            const float below_drag = vcb_ballistics_g7_drag_mps2(below_mps, 1.0f);
            const float above_drag = vcb_ballistics_g7_drag_mps2(above_mps, 1.0f);
            const size_t threshold_segment =
                    threshold_mps / VCB_BALLISTICS_FPS_TO_MPS > threshold_fps ?
                    i : i + 1u;
            const float expected_threshold = segment_drag(threshold_segment,
                                                           threshold_mps);
            const float expected_below = segment_drag(i + 1u, below_mps);
            const float expected_above = segment_drag(i, above_mps);
            const float upper_at_threshold = segment_drag(i, threshold_mps);
            const float lower_at_threshold = segment_drag(i + 1u, threshold_mps);
            const float jump = relative_delta(upper_at_threshold,
                                              lower_at_threshold);

            if(below_mps / VCB_BALLISTICS_FPS_TO_MPS == threshold_fps){
                exact_count++;
                check_true(relative_delta(below_drag, expected_below) <= 2.0e-6f,
                           "exact selects lower segment", i);
            }
            check_true(nextafterf(below_mps, INFINITY) == above_mps,
                       "adjacent inputs straddle threshold", i);
            check_true(relative_delta(threshold_drag, expected_threshold) <= 2.0e-6f,
                       "nominal threshold input selection", i);
            check_true(relative_delta(below_drag, expected_below) <= 2.0e-6f,
                       "lower adjacent selects lower segment", i);
            check_true(relative_delta(above_drag, expected_above) <= 2.0e-6f,
                       "upper adjacent selects upper segment", i);
            check_true(isfinite(threshold_drag) && isfinite(below_drag) &&
                       isfinite(above_drag), "finite adjacent drag", i);

            printf("  knot %zu at %6.0f fps (%7.2f m/s): upper %.6g vs lower "
                   "%.6g, jump %+7.3f%%\n", i, (double)threshold_fps,
                   (double)threshold_mps, (double)upper_at_threshold,
                   (double)lower_at_threshold,
                   100.0 * (double)(upper_at_threshold - lower_at_threshold) /
                   (double)lower_at_threshold);

            if(jump > worst_boundary_jump){
                worst_boundary_jump = jump;
                worst_boundary_index = i;
            }
        }
    }

    printf("G7 threshold audit: %u checks, %u failures, %zu exact thresholds\n",
           s_checks, s_failures, exact_count);
    printf("G7 worst fitted boundary jump: %.9g at %.0f fps (index %zu)\n",
           (double)worst_boundary_jump,
           (double)s_g7_segments[worst_boundary_index].min_velocity_fps,
           worst_boundary_index);
    return s_failures == 0u ? 0 : 1;
}
