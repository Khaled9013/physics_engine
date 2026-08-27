#include <math.h>
#include <stdio.h>

#include "../../source/vcb_ballistics.c"

#define VCB_FPS_TO_MPS_D 0.3048
/* production float drag vs double closed form agrees to 5.7e-07, so gate at ~9x */
#define VCB_CLOSED_FORM_GATE 5.0e-6

typedef struct {
    const char *name;
    vcb_ballistics_drag_family_e family;
    double coefficient;
    double exponent;
    double min_velocity_fps;
} vcb_segment_t;

/* G1 segment 0 is selected while velocity_fps > 4230, i.e. above 1289.3 m/s */
static const vcb_segment_t s_g1_top = {
    "G1 seg0", vcb_ballistics_drag_family_g1,
    1.477404177730177e-04, 1.9565, 4230.0
};
/* G7 segment 1 is selected for 3000 < velocity_fps <= 4200, i.e. 914.4 to 1280.2 m/s */
static const vcb_segment_t s_g7_mid = {
    "G7 seg1", vcb_ballistics_drag_family_g7,
    0.0171422231434847, 1.27907168025204, 3000.0
};

/* production drag is C*(v/F)^m/BC*F, so D(v) = k*v^m with k = C*F^(1-m)/BC */
static double vcb_k(const vcb_segment_t *segment, double ballistic_coefficient)
{
    return segment->coefficient *
           pow(VCB_FPS_TO_MPS_D, 1.0 - segment->exponent) / ballistic_coefficient;
}

/* dv/dx = -k*v^(m-1) integrates to v(x) = [v0^(2-m) - (2-m)*k*x]^(1/(2-m)) */
static double vcb_velocity_exact(const vcb_segment_t *segment, double v0,
                                 double bc, double x)
{
    const double e = 2.0 - segment->exponent;
    return pow(pow(v0, e) - e * vcb_k(segment, bc) * x, 1.0 / e);
}

/* dt/dx = 1/v integrates to t(x) = (v0^(1-m) - v(x)^(1-m)) / (k*(1-m)) */
static double vcb_time_exact(const vcb_segment_t *segment, double v0,
                             double bc, double x)
{
    const double e = 1.0 - segment->exponent;
    return (pow(v0, e) - pow(vcb_velocity_exact(segment, v0, bc, x), e)) /
           (vcb_k(segment, bc) * e);
}

static unsigned int vcb_check(const char *what, double got, double want,
                              double *worst)
{
    const double relative = fabs(got - want) / fabs(want);

    if(relative > *worst){
        *worst = relative;
    }
    if(!(relative <= VCB_CLOSED_FORM_GATE)){
        printf("FAIL %s got %.12g want %.12g relative %.9g exceeds %.9g\n",
               what, got, want, relative, (double)VCB_CLOSED_FORM_GATE);
        return 1u;
    }
    return 0u;
}

/* run the production RK4 step over a gravity-only-perturbed horizontal shot */
static unsigned int vcb_run_case(const vcb_segment_t *segment, double v0,
                                 double bc, double distance_m, double *worst)
{
    vcb_ballistics_state_t state;
    double travelled_m = 0.0;
    double exact_velocity;
    unsigned int failures = 0u;
    char label[96];

    state.y = 0.0f;
    state.z = 0.0f;
    state.vx = (float)v0;
    state.vy = 0.0f;
    state.vz = 0.0f;
    state.time = 0.0f;

    while(travelled_m < distance_m - 1.0e-9){
        double step_m = distance_m - travelled_m;

        if(step_m > (double)VCB_BALLISTICS_STEP_M){
            step_m = (double)VCB_BALLISTICS_STEP_M;
        }
        if(vcb_ballistics_integrate_step(&state, (float)step_m, 0.0f, 0.0f,
                                         (float)bc,
                                         segment->family) !=
           vcb_ballistics_status_ok){
            printf("FAIL %s integrate step at %.3f m\n", segment->name,
                   travelled_m);
            return 1u;
        }
        travelled_m += step_m;
    }

    exact_velocity = vcb_velocity_exact(segment, v0, bc, distance_m);
    /* the whole run must stay inside one segment for the closed form to apply */
    if(!(exact_velocity / VCB_FPS_TO_MPS_D > segment->min_velocity_fps)){
        printf("FAIL %s left its segment at %.1f fps\n", segment->name,
               exact_velocity / VCB_FPS_TO_MPS_D);
        return 1u;
    }

    printf("  %s v0=%6.0f bc=%4.2f over %5.0f m -> %.1f fps: vx %.9g vs %.9g\n",
           segment->name, v0, bc, distance_m,
           exact_velocity / VCB_FPS_TO_MPS_D, (double)state.vx, exact_velocity);
    sprintf(label, "%s velocity v0=%.0f bc=%.2f", segment->name, v0, bc);
    failures += vcb_check(label, (double)state.vx, exact_velocity, worst);
    sprintf(label, "%s time v0=%.0f bc=%.2f", segment->name, v0, bc);
    failures += vcb_check(label, (double)state.time,
                          vcb_time_exact(segment, v0, bc, distance_m), worst);
    return failures;
}

static unsigned int vcb_drag_law(const vcb_segment_t *segment, double from_mps,
                                 double step_mps, double *worst)
{
    unsigned int failures = 0u;
    int i;

    for(i = 0; i < 20; i++){
        const double velocity = from_mps + step_mps * (double)i;
        const double got = (double)vcb_ballistics_drag_mps2(
                segment->family, (float)velocity, 2.0f);
        char label[64];

        sprintf(label, "%s drag law at %.0f m/s", segment->name, velocity);
        failures += vcb_check(label, got,
                              vcb_k(segment, 2.0) *
                              pow(velocity, segment->exponent), worst);
    }
    return failures;
}

int main(void)
{
    unsigned int checks = 0u;
    unsigned int failures = 0u;
    double worst = 0.0;

    printf("Power law, production float drag vs double closed form:\n");
    checks += 40u;
    failures += vcb_drag_law(&s_g1_top, 1300.0, 10.0, &worst);
    failures += vcb_drag_law(&s_g7_mid, 950.0, 15.0, &worst);
    printf("  20 velocities per family checked\n");

    printf("Production RK4 vs closed-form integral:\n");
    checks += 12u;
    failures += vcb_run_case(&s_g1_top, 1500.0, 2.0, 200.0, &worst);
    failures += vcb_run_case(&s_g1_top, 1500.0, 1.0, 100.0, &worst);
    failures += vcb_run_case(&s_g1_top, 1450.0, 2.0, 150.0, &worst);
    failures += vcb_run_case(&s_g7_mid, 1270.0, 2.0, 400.0, &worst);
    failures += vcb_run_case(&s_g7_mid, 1270.0, 0.05f, 80.0, &worst);
    failures += vcb_run_case(&s_g7_mid, 1100.0, 0.5, 200.0, &worst);

    printf("\nWorst closed-form relative error %.9g, gate %.9g\n", worst,
           (double)VCB_CLOSED_FORM_GATE);
    printf("Closed form: %u checks, %u failures\n", checks, failures);
    return failures == 0u ? 0 : 1;
}
