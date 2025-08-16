/* Copyright 2024 ai03 Design Studio */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H
#include "keymap_japanese.h"  // JP_* keycodes for Japanese (JIS) layout

// ---------- Custom keycodes ----------
enum custom_keycodes {
    TAB_BELOW = SAFE_RANGE,  // macOS: LCTL (modifier), iOS/Windows/Linux: MO(_EMACS)
    // US emulation targets: keys that differ on JIS
    US_2, US_6, US_7, US_8, US_9, US_0,   // digits with shifted symbols
    US_MINS, US_EQL, US_SCLN, US_QUOT, US_GRV,
    US_LBRC, US_RBRC, US_BSLS,

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
    OS_PGDN   // mac/iOS: Alt+Down (approx page down), else KC_PGDN
};

// ---------- Layers ----------
enum layers {
    _BASE = 0,
    _NAVIGATION,  // Layer 1: Navigation and arrows
    _EMACS,       // Layer 2: Emacs-like cursor/editing (Windows/iOS via TAB_BELOW)
    _MODIFIER     // Layer 3: Modifier layer for EISU hold
};

// Layer-taps for JP IME ergonomics
#define KANA_L1 LT(_NAVIGATION, KC_LNG1)  // tap=Kana (LNG1), hold=Navigation
#define EISU_LT LT(_MODIFIER,  KC_LNG2)   // tap=Eisu (LNG2), hold=Modifier

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
static inline bool any_shift(void) {
    return (get_mods() | get_oneshot_mods()) & MOD_MASK_SHIFT;
}

// Send kc once with Shift fully suppressed (incl. one-shots), then restore mods.
static void tap_unshifted(uint16_t kc) {
    uint8_t mods    = get_mods();
    uint8_t oneshot = get_oneshot_mods();
    del_mods(MOD_MASK_SHIFT);
    clear_oneshot_mods();
    send_keyboard_report();
    tap_code16(kc);
    set_mods(mods);
    set_oneshot_mods(oneshot);
    send_keyboard_report();
}

// Wrapper: if emulating and Shift held, send shifted_jp with Shift suppressed;
// otherwise send unshifted_kc as-is.
static void tap_with_jp_shift(bool pressed, uint16_t unshifted_kc, uint16_t shifted_jp) {
    if (!pressed) return;
    if (g_us_emulate && any_shift()) {
        tap_unshifted(shifted_jp);
    } else {
        tap_code16(unshifted_kc);
    }
}

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

// ---------- Keymaps ----------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_ESC,  KC_1,    US_2,    KC_3,    KC_4,    KC_5,    US_EQL,  US_EQL,  US_6,    US_7,    US_8,    US_9,    US_0,    KC_BSPC,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_LPRN, US_LBRC, KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    US_MINS,
        TAB_BELOW, KC_A,  KC_S,    KC_D,    KC_F,    KC_G,    KC_RPRN, US_RBRC, KC_H,    KC_J,    KC_K,    KC_L,    US_SCLN, US_QUOT,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    US_GRV,  US_BSLS, KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_ENT,
                                   KC_LGUI, KC_LALT, EISU_LT, KC_SPC,  KC_SPC,  KANA_L1, KANA_L1,  KC_BTN2
    ),

    [_NAVIGATION] = LAYOUT(
        QK_BOOT, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_PWR,
        _______, _______, _______, _______, _______, _______, _______, KC_BTN4, KC_HOME, KC_PGDN, KC_PGUP, KC_END,  _______, _______,
        _______, MAC,     IOS,     WIN_US,  WIN_JIS, _______, _______, KC_BTN5, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, KC_HOME, KC_PGDN, KC_PGUP, KC_END,  _______, KC_ESC,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_EMACS] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, KC_UP,   OS_END,  _______, _______, _______, _______, OS_PGUP, _______, _______, _______, KC_UP,   _______,
        _______, OS_HOME, _______, KC_DEL,  KC_RGHT, _______, _______, _______, KC_BSPC, KC_ENT,  KILL_EOL, _______, _______, _______,
        _______, _______, _______, _______, OS_PGDN, KC_LEFT, _______, _______, KC_DOWN, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_MODIFIER] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    )
};

// ---------- Key processing ----------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        // macOS: act as Control; iOS/Windows/Linux: momentary EMACS layer
        case TAB_BELOW:
            // On iOS, behave like Windows/Linux: momentary EMACS layer
            if (g_is_mac && !g_is_ios) {
                if (record->event.pressed) { register_mods(MOD_BIT(KC_LCTL)); }
                else                        { unregister_mods(MOD_BIT(KC_LCTL)); }
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

        // ----- Digit row: unshifted digits (KC_*), shifted symbols via JP_* when emulating -----
        case US_2: tap_with_jp_shift(record->event.pressed, KC_2, JP_AT);      return false;  // 2 / @
        case US_6: tap_with_jp_shift(record->event.pressed, KC_6, JP_CIRC);    return false;  // 6 / ^
        case US_7: tap_with_jp_shift(record->event.pressed, KC_7, JP_AMPR);    return false;  // 7 / &
        case US_8: tap_with_jp_shift(record->event.pressed, KC_8, JP_ASTR);    return false;  // 8 / *
        case US_9: tap_with_jp_shift(record->event.pressed, KC_9, JP_LPRN);    return false;  // 9 / (
        case US_0: tap_with_jp_shift(record->event.pressed, KC_0, JP_RPRN);    return false;  // 0 / )

        // ----- Punctuation: KC_* on macOS, JP_* on Windows for unshifted; JP_* for shifted when emulating -----
        case US_MINS: tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_MINS : JP_MINS, JP_UNDS);  return false; // - / _
        case US_EQL:  tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_EQL  : JP_EQL,  JP_PLUS);  return false; // = / +
        case US_SCLN: tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_SCLN : JP_SCLN, JP_COLN);  return false; // ; / :
        case US_QUOT: tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_QUOT : JP_QUOT, JP_DQUO);  return false; // ' / "
        case US_GRV:  tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_GRV  : JP_GRV,  JP_TILD);  return false; // ` / ~

        // ----- Brackets & Backslash -----
        case US_LBRC: tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_LBRC : JP_LBRC, JP_LCBR);  return false; // [ / {
        case US_RBRC: tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_RBRC : JP_RBRC, JP_RCBR);  return false; // ] / }
        case US_BSLS: tap_with_jp_shift(record->event.pressed, g_is_mac ? KC_BSLS : JP_BSLS, JP_PIPE);  return false; // \ / |
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

