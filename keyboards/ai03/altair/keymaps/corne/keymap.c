/* Copyright 2024 ai03 Design Studio */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H
#include "keymap_japanese.h"  // JP_* keycodes for Japanese (JIS) layout

// ---------- Custom keycodes ----------
enum custom_keycodes {
    TAB_BELOW = SAFE_RANGE,  // macOS: hold Control (+ temporary _ESC); iOS/Windows/Linux: momentary _EMACS

    // US emulation targets: keys that differ on JIS (punctuation & number-row symbols)
    US_MINS, US_EQL, US_LBRC, US_RBRC, US_BSLS,
    US_SCLN, US_QUOT, US_GRV,  US_COMM, US_DOT,  US_SLSH,
    US_TILD, US_EXLM, US_AT,   US_HASH, US_DLR,  US_PERC,
    US_CIRC, US_AMPR, US_ASTR, US_LPRN, US_RPRN,
    US_PLUS, US_PIPE,

    // Manual mode selectors on Navigation layer (a/s/d/f)
    MAC,
    IOS,
    WIN_US,   // Windows recognized as US (no emulation)
    WIN_JIS,  // Windows recognized as JIS; emulate US symbols

    // Editing helpers
    KILL_EOL, // Emacs-like: kill to end of line

    // OS-aware navigation for Emacs layer
    OS_HOME,  // mac/iOS: Cmd+Left, else KC_HOME
    OS_END,   // mac/iOS: Cmd+Right, else KC_END
    OS_PGUP,  // mac/iOS: Alt+Up (approx page up), else KC_PGUP
    OS_PGDN,  // mac/iOS: Alt+Down (approx page down), else KC_PGDN

    // Plain Escape (used on _ESC to avoid Ctrl+Esc when TAB_BELOW holds Ctrl)
    ESC_PLAIN
};

// ---------- Tap Dance IDs ----------
enum {
    TD_Q_ESC = 0,
};

#define Q_ESC TD(TD_Q_ESC) // Tap Dance: Q -> Esc (double-tap Esc)

// ---------- Layers ----------
enum layers {
    _BASE = 0,
    _NAVIGATION,  // Layer 1: Navigation and arrows
    _EMACS,       // Layer 2: Emacs-like cursor/editing (Windows/iOS via TAB_BELOW)
    _NUMBER,      // Layer 3: Numbers & symbols
    _FUNCTION,    // Layer 4: F1-F12
    _MODIFIER,    // Layer 5: OS-specific modifier while holding EISU
    _ESC      // Layer 6: helper while TAB_BELOW is held on macOS (Enter -> plain Esc only)
};

// Layer-taps for JP IME ergonomics
#define KANA_L1 LT(_NUMBER, KC_LNG1)      // tap=Kana (LNG1), hold=Number
#define EISU_LT LT(_MODIFIER, KC_LNG2)    // tap=Eisu (LNG2), hold=Modifier
#define NAV MO(_NAVIGATION)               // momentary Navigation layer
#define FUNC LT(_FUNCTION, KC_SPC)        // tap=Function (F1-F12), hold=Space

// ---------- Runtime state ----------
static bool g_is_mac      = false; // macOS or iOS (Apple family)
static bool g_is_ios      = false; // iOS only
static bool g_us_emulate  = false; // enable US emulation only when Windows+JIS (manual f)
static bool g_manual_mode = false; // when true, ignore OS detection and use manual selection

// ---------- Mode application helper ----------
static void apply_mode(bool is_mac, bool is_ios, bool us_emulate, bool manual) {
    g_is_mac      = is_mac;
    g_is_ios      = is_ios;
    g_us_emulate  = us_emulate;
    g_manual_mode = manual;
}

// ---------- OS detection (requires: OS_DETECTION_ENABLE = yes in rules.mk) ----------
// Respect manual override: if user selected a mode, do not change it automatically.
// IMPORTANT: On Windows, default to NO emulation because the host may already see the board as US.
// If the host is Windows+JIS and you want US-like symbols, press 'f' (WIN_JIS) on the Navigation layer.
bool process_detected_host_os_user(os_variant_t os) {
    if (g_manual_mode) return true;

    const bool is_mac_family = (os == OS_MACOS || os == OS_IOS);
    const bool is_ios        = (os == OS_IOS);
    const bool default_us_emulation = false;  // never assume emulation automatically on Windows

    apply_mode(is_mac_family, is_ios, default_us_emulation, false);
    return true;
}

// ---------- Helpers ----------

// Emacs-like kill to end of line: select to EOL then delete.
// macOS/iOS: GUI+Shift+Right, then Delete
// Windows/Linux: Shift+End, then Delete
static void do_kill_eol(void) {
    if (g_is_mac) {
        register_code(KC_LGUI);
        register_code(KC_LSFT);
        tap_code(KC_RGHT);
        unregister_code(KC_LSFT);
        unregister_code(KC_LGUI);
        tap_code(KC_DEL);
    } else {
        tap_code16(S(KC_END));
        tap_code(KC_DEL);
    }
}

// OS-aware navigation helpers used on the Emacs layer
static void do_os_home(void) {
    if (g_is_mac) {
        register_code(KC_LGUI); tap_code(KC_LEFT); unregister_code(KC_LGUI); // Cmd+Left -> line start
    } else {
        tap_code(KC_HOME);
    }
}
static void do_os_end(void) {
    if (g_is_mac) {
        register_code(KC_LGUI); tap_code(KC_RGHT); unregister_code(KC_LGUI); // Cmd+Right -> line end
    } else {
        tap_code(KC_END);
    }
}
static void do_os_pgup(void) {
    if (g_is_mac) {
        register_code(KC_LALT); tap_code(KC_UP); unregister_code(KC_LALT);   // Alt+Up -> paragraph/page-ish up
    } else {
        tap_code(KC_PGUP);
    }
}
static void do_os_pgdn(void) {
    if (g_is_mac) {
        register_code(KC_LALT); tap_code(KC_DOWN); unregister_code(KC_LALT); // Alt+Down -> paragraph/page-ish down
    } else {
        tap_code(KC_PGDN);
    }
}

// Send helper that always supports 16-bit keycodes
static void tap_us_or_jp(uint16_t kc_us, uint16_t kc_jp) {
    if (g_us_emulate) {
        tap_code16(kc_jp);   // On Windows+JIS, send JP_* to produce US-like glyphs
    } else {
        tap_code16(kc_us);   // Otherwise, send the normal US-oriented KC_*
    }
}

// ---------- Tap Dance actions ----------
tap_dance_action_t tap_dance_actions[] = {
    [TD_Q_ESC] = ACTION_TAP_DANCE_DOUBLE(KC_Q, KC_ESC),
};

// ---------- Keymaps ----------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        KC_TAB,  Q_ESC,   KC_W,    KC_E,    KC_R,    KC_T,    _______, US_LBRC, KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    US_MINS,
        TAB_BELOW, KC_A,  KC_S,    KC_D,    KC_F,    KC_G,    KC_LGUI, US_RBRC, KC_H,    KC_J,    KC_K,    KC_L,    US_SCLN, US_QUOT,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    _______, _______, KC_N,    KC_M,    US_COMM, US_DOT,  US_SLSH, KC_ENT,
                                   _______, KC_LALT, EISU_LT, KC_BSPC, FUNC,    KANA_L1, NAV,     _______
    ),

    [_NAVIGATION] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, KC_BTN4, KC_HOME, KC_PGDN, KC_PGUP, KC_END,  _______, _______,
        _______, MAC,     IOS,     WIN_US,  WIN_JIS, _______, _______, KC_BTN5, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, KC_HOME, KC_PGDN, KC_PGUP, KC_END,  _______, KC_ESC,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_EMACS] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, KC_UP,   OS_END,  _______, _______, _______, _______, OS_PGUP, _______, _______, _______, KC_UP,   KC_ESC,
        _______, OS_HOME, _______, KC_DEL,  KC_RGHT, _______, _______, _______, KC_BSPC, KC_ENT,  KILL_EOL, _______, _______, _______,
        _______, _______, _______, _______, OS_PGDN, KC_LEFT, _______, _______, KC_DOWN, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_NUMBER] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        US_TILD, US_EXLM, US_AT,   US_HASH, US_DLR,  US_PERC, _______, KC_BTN4, US_CIRC, US_AMPR, US_ASTR, US_LPRN, US_RPRN, US_PLUS,
        US_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    _______, KC_BTN5, KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    US_EQL,
        _______, US_PLUS, US_MINS, US_ASTR, US_SLSH, US_EQL, _______, _______, _______, _______,  US_LBRC, US_RBRC, US_BSLS, US_PIPE,
                                   _______, _______, _______, KC_DEL,  _______, _______, _______, _______
    ),

    [_FUNCTION] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        QK_BOOT, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_PWR,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_MODIFIER] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_ESC] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, ESC_PLAIN,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    )
};

// ---------- Key processing ----------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        // macOS: hold Control and turn on _ESC so bottom-right Enter acts as plain Esc while held
        // iOS/Windows/Linux: momentary _EMACS layer
        case TAB_BELOW:
            if (g_is_mac && !g_is_ios) {
                if (record->event.pressed) {
                    register_mods(MOD_BIT(KC_LCTL));   // hold Ctrl on macOS
                    layer_on(_ESC);                // enable helper layer (Enter -> plain Esc)
                } else {
                    unregister_mods(MOD_BIT(KC_LCTL)); // release Ctrl
                    layer_off(_ESC);               // disable helper layer
                }
            } else {
                if (record->event.pressed) { layer_on(_EMACS); }
                else                        { layer_off(_EMACS); }
            }
            return false;

        // ----- Manual mode selectors on Navigation layer (a/s/d/f) -----
        case MAC:
            if (record->event.pressed) {
                // macOS mode: mac semantics, not iOS, no US emulation
                apply_mode(true, false, false, true);
            }
            return false;

        case IOS:
            if (record->event.pressed) {
                // iOS/iPadOS mode: treated as Apple, with iOS-specific TAB_BELOW behavior
                apply_mode(true, true, false, true);
            }
            return false;

        case WIN_US:
            if (record->event.pressed) {
                // Windows (US layout): host recognizes the board as US; no emulation required
                apply_mode(false, false, false, true);
            }
            return false;

        case WIN_JIS:
            if (record->event.pressed) {
                // Windows (JIS layout): enable US emulation so shifted symbols behave like US
                apply_mode(false, false, true, true);
            }
            return false;

        // ----- Emacs editing helpers -----
        case KILL_EOL:
            if (record->event.pressed) {
                do_kill_eol();
            }
            return false;

        // ----- OS-aware navigation (Emacs layer) -----
        case OS_HOME:
            if (record->event.pressed) do_os_home();
            return false;
        case OS_END:
            if (record->event.pressed) do_os_end();
            return false;
        case OS_PGUP:
            if (record->event.pressed) do_os_pgup();
            return false;
        case OS_PGDN:
            if (record->event.pressed) do_os_pgdn();
            return false;

        // ----- Plain Esc sender for _ESC -----
        case ESC_PLAIN:
            if (record->event.pressed) {
                // Save current modifier states
                uint8_t mods      = get_mods();
                uint8_t weak_mods = get_weak_mods();
                uint8_t osm       = get_oneshot_mods();

                // Temporarily remove Ctrl from all buckets
                del_mods(MOD_MASK_CTRL);
                del_weak_mods(MOD_MASK_CTRL);
                del_oneshot_mods(MOD_MASK_CTRL);
                send_keyboard_report();

                // Send a plain Escape (no Ctrl)
                tap_code(KC_ESC);

                // Restore previous modifier states
                set_mods(mods);
                set_weak_mods(weak_mods);
                set_oneshot_mods(osm);
                send_keyboard_report();
            }
            return false;

        // ----- US_* codes for JIS emulation (punctuation & number-row symbols) -----
        case US_MINS:
            if (record->event.pressed) tap_us_or_jp(KC_MINS, JP_MINS);
            return false;
        case US_EQL:
            if (record->event.pressed) tap_us_or_jp(KC_EQL, JP_EQL);
            return false;
        case US_LBRC:
            if (record->event.pressed) tap_us_or_jp(KC_LBRC, JP_LBRC);
            return false;
        case US_RBRC:
            if (record->event.pressed) tap_us_or_jp(KC_RBRC, JP_RBRC);
            return false;
        case US_BSLS:
            if (record->event.pressed) tap_us_or_jp(KC_BSLS, JP_BSLS);
            return false;
        case US_SCLN:
            if (record->event.pressed) tap_us_or_jp(KC_SCLN, JP_SCLN);
            return false;
        case US_QUOT:
            if (record->event.pressed) tap_us_or_jp(KC_QUOT, JP_QUOT);
            return false;
        case US_GRV:
            if (record->event.pressed) tap_us_or_jp(KC_GRV, JP_GRV);
            return false;
        case US_COMM:
            if (record->event.pressed) tap_us_or_jp(KC_COMM, JP_COMM);
            return false;
        case US_DOT:
            if (record->event.pressed) tap_us_or_jp(KC_DOT, JP_DOT);
            return false;
        case US_SLSH:
            if (record->event.pressed) tap_us_or_jp(KC_SLSH, JP_SLSH);
            return false;
        case US_PLUS:
            if (record->event.pressed) tap_us_or_jp(KC_PLUS, JP_PLUS);
            return false;
        case US_PIPE:
            if (record->event.pressed) tap_us_or_jp(KC_PIPE, JP_PIPE);
            return false;

        // Number-row shifted symbols aligned to US layout
        case US_TILD:
            if (record->event.pressed) tap_us_or_jp(KC_TILD, JP_GRV);
            return false;
        case US_EXLM:
            if (record->event.pressed) tap_us_or_jp(KC_EXLM, JP_EXLM);
            return false;
        case US_AT:
            if (record->event.pressed) tap_us_or_jp(KC_AT, JP_AT);
            return false;
        case US_HASH:
            if (record->event.pressed) tap_us_or_jp(KC_HASH, JP_HASH);
            return false;
        case US_DLR:
            if (record->event.pressed) tap_us_or_jp(KC_DLR, JP_DLR);
            return false;
        case US_PERC:
            if (record->event.pressed) tap_us_or_jp(KC_PERC, JP_PERC);
            return false;
        case US_CIRC:
            if (record->event.pressed) tap_us_or_jp(KC_CIRC, JP_CIRC);
            return false;
        case US_AMPR:
            if (record->event.pressed) tap_us_or_jp(KC_AMPR, JP_AMPR);
            return false;
        case US_ASTR:
            if (record->event.pressed) tap_us_or_jp(KC_ASTR, JP_ASTR);
            return false;
        case US_LPRN:
            if (record->event.pressed) tap_us_or_jp(KC_LPRN, JP_LPRN);
            return false;
        case US_RPRN:
            if (record->event.pressed) tap_us_or_jp(KC_RPRN, JP_RPRN);
            return false;
    }
    return true;
}

// ---------- Modifier layer: OS-specific mod while holding EISU_LT ----------
layer_state_t layer_state_set_user(layer_state_t state) {
    if (IS_LAYER_ON_STATE(state, _MODIFIER)) {
        if (g_is_mac) register_mods(MOD_BIT(KC_LGUI)); // Cmd on macOS/iOS
        else          register_mods(MOD_BIT(KC_LCTL)); // Ctrl on Windows/Linux
    } else {
        if (g_is_mac) unregister_mods(MOD_BIT(KC_LGUI));
        else          unregister_mods(MOD_BIT(KC_LCTL));
    }
    return state;
}
