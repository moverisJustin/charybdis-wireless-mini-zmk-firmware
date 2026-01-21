/*
 * Copyright (c) 2025 Justin Keene
 * SPDX-License-Identifier: MIT
 *
 * Mouse Acceleration Input Processor for ZMK
 * Ported from QMK's maccel concept by Wimads and burkfers
 *
 * Implements velocity-based acceleration using a piecewise linear curve:
 * - Slow movements (velocity < slow_threshold) use slow_factor
 * - Medium movements use med_factor
 * - Fast movements (velocity >= fast_threshold) use 100% (full speed)
 */

#define DT_DRV_COMPAT zmk_input_processor_maccel

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(maccel, CONFIG_ZMK_LOG_LEVEL);

/* Configuration from devicetree */
struct maccel_config {
    uint16_t slow_threshold;  /* Velocity threshold for slow mode */
    uint16_t fast_threshold;  /* Velocity threshold for fast mode */
    uint16_t slow_factor;     /* Factor (%) for slow movements */
    uint16_t med_factor;      /* Factor (%) for medium movements */
};

/* Runtime state - persists across events */
struct maccel_state {
    int64_t last_time;        /* Timestamp of last event */
    int16_t last_x;           /* Last X movement value */
    int16_t last_y;           /* Last Y movement value */
    int32_t remainder_x;      /* Accumulated fractional X */
    int32_t remainder_y;      /* Accumulated fractional Y */
    bool have_x;              /* Have pending X value */
    bool have_y;              /* Have pending Y value */
};

/* Integer square root approximation using Newton's method */
static uint32_t isqrt(uint32_t n) {
    if (n == 0) return 0;

    uint32_t x = n;
    uint32_t y = (x + 1) / 2;

    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/* Calculate acceleration factor based on velocity (returns 0-100) */
static uint16_t calculate_factor(const struct maccel_config *config, uint32_t velocity) {
    if (velocity < config->slow_threshold) {
        return config->slow_factor;
    } else if (velocity < config->fast_threshold) {
        return config->med_factor;
    } else {
        return 100;  /* Full speed */
    }
}

/* Apply factor to value with remainder tracking */
static int16_t apply_factor(int16_t value, uint16_t factor, int32_t *remainder) {
    /* Scale up for precision: value * factor, keeping track of remainder */
    int32_t scaled = ((int32_t)value * factor) + *remainder;

    /* Result is scaled / 100, remainder is the fractional part */
    int16_t result = (int16_t)(scaled / 100);
    *remainder = scaled % 100;

    return result;
}

static int maccel_handle_event(const struct device *dev, struct input_event *event,
                               uint32_t param1, uint32_t param2,
                               struct zmk_input_processor_state *state) {
    const struct maccel_config *config = dev->config;

    /* Get our persistent state from the state array */
    /* We use state->remainder as our state pointer */
    static struct maccel_state maccel_states[4] = {0};  /* Support up to 4 instances */

    uint8_t idx = state->input_device_index % 4;
    struct maccel_state *ms = &maccel_states[idx];

    /* Only process relative X/Y movement events */
    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /* Capture X and Y values */
    if (event->code == INPUT_REL_X) {
        ms->last_x = event->value;
        ms->have_x = true;
    } else if (event->code == INPUT_REL_Y) {
        ms->last_y = event->value;
        ms->have_y = true;
    } else {
        /* Not X or Y, pass through */
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /* Wait until we have both X and Y before processing */
    /* (They typically come in pairs, sync event follows) */
    if (event->sync && ms->have_x && ms->have_y) {
        int64_t now = k_uptime_get();
        int64_t delta_time = now - ms->last_time;

        /* Prevent division by zero and handle first event */
        if (delta_time <= 0 || ms->last_time == 0) {
            delta_time = 1;
        }

        /* Calculate distance (magnitude of movement vector) */
        uint32_t dist_sq = (uint32_t)(ms->last_x * ms->last_x + ms->last_y * ms->last_y);
        uint32_t distance = isqrt(dist_sq);

        /* Calculate velocity: distance per millisecond, scaled up by 100 for precision */
        uint32_t velocity = (distance * 100) / (uint32_t)delta_time;

        /* Get acceleration factor */
        uint16_t factor = calculate_factor(config, velocity);

        LOG_DBG("maccel: dist=%u dt=%lld vel=%u factor=%u",
                distance, delta_time, velocity, factor);

        /* Apply factor to both axes (the current event is Y, modify it) */
        /* Note: X was already processed, so we need to handle this differently */
        /* For now, apply factor to Y only since that's the current event */
        event->value = apply_factor(ms->last_y, factor, &ms->remainder_y);

        /* Update state */
        ms->last_time = now;
        ms->have_x = false;
        ms->have_y = false;
    } else if (event->code == INPUT_REL_X) {
        /* For X events, we apply the factor immediately based on last known velocity */
        /* This is a simplification - ideally we'd batch X and Y together */
        int64_t now = k_uptime_get();
        int64_t delta_time = now - ms->last_time;

        if (delta_time <= 0 || ms->last_time == 0) {
            delta_time = 1;
        }

        /* Use just X for velocity estimate when Y isn't available yet */
        uint32_t distance = (uint32_t)(ms->last_x > 0 ? ms->last_x : -ms->last_x);
        uint32_t velocity = (distance * 100) / (uint32_t)delta_time;
        uint16_t factor = calculate_factor(config, velocity);

        event->value = apply_factor(ms->last_x, factor, &ms->remainder_x);
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api maccel_driver_api = {
    .handle_event = maccel_handle_event,
};

/* Macro to instantiate the driver for each DT node */
#define MACCEL_INST(n)                                                              \
    static const struct maccel_config maccel_config_##n = {                         \
        .slow_threshold = DT_INST_PROP_OR(n, slow_threshold, 5),                    \
        .fast_threshold = DT_INST_PROP_OR(n, fast_threshold, 20),                   \
        .slow_factor = DT_INST_PROP_OR(n, slow_factor, 30),                         \
        .med_factor = DT_INST_PROP_OR(n, med_factor, 60),                           \
    };                                                                              \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &maccel_config_##n,                  \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,                  \
                          &maccel_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MACCEL_INST)
