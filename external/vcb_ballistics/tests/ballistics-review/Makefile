CC ?= cc
BUILD_DIR ?= /tmp/vcb-ballistics-review

SOLVER := ../../source/vcb_ballistics.c
HEADER := ../../source/vcb_ballistics.h

CPPFLAGS += -I../../source
WARNFLAGS := -Wall -Wextra -Wpedantic -Werror
CFLAGS += -std=c11 -O2 $(WARNFLAGS)
LDLIBS += -lm

SANFLAGS := -fsanitize=address,undefined
SANCFLAGS := -std=c11 -O1 -g -fno-omit-frame-pointer $(SANFLAGS) $(WARNFLAGS)

# refined-step variants rename the entry point so one binary can compare step sizes
DEFS_025 := -Dvcb_ballistics_solve=vcb_ballistics_solve_step_025 -DVCB_BALLISTICS_STEP_M=0.25f
DEFS_0125 := -Dvcb_ballistics_solve=vcb_ballistics_solve_step_0125 -DVCB_BALLISTICS_STEP_M=0.125f
DEFS_00625 := -Dvcb_ballistics_solve=vcb_ballistics_solve_step_00625 -DVCB_BALLISTICS_STEP_M=0.0625f

OBJ_PROD := $(BUILD_DIR)/vcb_ballistics.o
OBJ_025 := $(BUILD_DIR)/vcb_ballistics_step_025.o
OBJ_0125 := $(BUILD_DIR)/vcb_ballistics_step_0125.o
OBJ_00625 := $(BUILD_DIR)/vcb_ballistics_step_00625.o
OBJS := $(OBJ_PROD) $(OBJ_025) $(OBJ_0125) $(OBJ_00625)

TEST_REVIEW := $(BUILD_DIR)/test_ballistics_review
TEST_G1 := $(BUILD_DIR)/test_g1_thresholds
TEST_G7 := $(BUILD_DIR)/test_g7_thresholds
TEST_KNOT := $(BUILD_DIR)/test_g7_knot
TEST_ZERO := $(BUILD_DIR)/test_zero_bisection
TEST_ORDER := $(BUILD_DIR)/test_convergence_order
TEST_CLOSED := $(BUILD_DIR)/test_closed_form
BENCH := $(BUILD_DIR)/benchmark_ballistics
TESTS := $(TEST_REVIEW) $(TEST_G1) $(TEST_G7) $(TEST_KNOT) $(TEST_ZERO) $(TEST_ORDER) $(TEST_CLOSED)

.PHONY: all run sanitize bench clean

all: $(TESTS) $(BENCH)

$(OBJ_PROD): $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(SOLVER) -o $@

$(OBJ_025): $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(DEFS_025) $(CFLAGS) -c $(SOLVER) -o $@

$(OBJ_0125): $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(DEFS_0125) $(CFLAGS) -c $(SOLVER) -o $@

$(OBJ_00625): $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(DEFS_00625) $(CFLAGS) -c $(SOLVER) -o $@

$(TEST_REVIEW): test_ballistics_review.c $(OBJS) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_ballistics_review.c $(OBJS) $(LDLIBS) -o $@

$(TEST_ZERO): test_zero_bisection.c $(OBJ_PROD) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_zero_bisection.c $(OBJ_PROD) $(LDLIBS) -o $@

# includes the solver translation unit directly, so it never links a solver object
$(TEST_G1): test_g1_thresholds.c $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_g1_thresholds.c $(LDLIBS) -o $@

$(TEST_G7): test_g7_thresholds.c $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_g7_thresholds.c $(LDLIBS) -o $@

$(TEST_KNOT): test_g7_knot.c $(OBJS) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_g7_knot.c $(OBJS) $(LDLIBS) -o $@

$(TEST_ORDER): test_convergence_order.c $(OBJS) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_convergence_order.c $(OBJS) $(LDLIBS) -o $@

# reaches solver internals, so it includes the translation unit like the G1 test
$(TEST_CLOSED): test_closed_form.c $(SOLVER) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) test_closed_form.c $(LDLIBS) -o $@

$(BENCH): benchmark_ballistics.c $(OBJ_PROD) $(HEADER)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) benchmark_ballistics.c $(OBJ_PROD) $(LDLIBS) -o $@

run: $(TESTS)
	$(TEST_REVIEW)
	$(TEST_G1)
	$(TEST_G7)
	$(TEST_KNOT)
	$(TEST_ZERO)
	$(TEST_ORDER)
	$(TEST_CLOSED)

sanitize:
	rm -rf $(BUILD_DIR)/asan
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/asan CFLAGS="$(SANCFLAGS)" LDFLAGS="$(SANFLAGS)" run

bench: $(BENCH)
	$(BENCH)

clean:
	rm -rf $(BUILD_DIR)
