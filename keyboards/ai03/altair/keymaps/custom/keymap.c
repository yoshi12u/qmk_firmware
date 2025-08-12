/* Copyright 2024 ai03 Design Studio */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H

// ---------- Custom keycodes ----------
enum custom_keycodes {
    TAB_BELOW = SAFE_RANGE,  // macOS: LCTL (modifier), Windows: MO(_EMACS)
};

// ---------- Layers ----------
enum layers {
    _BASE = 0,
    _NAVIGATION,  // Layer 2: Navigation and arrows
    _EMACS,       // Layer 3: Emacs-like cursor/editing (Windows via TAB_BELOW)
    _MODIFIER     // Layer 4: Modifier layer for EISU hold
};

// Built-in Layer-Tap for Kana: tap = KC_LNG1, hold = _NAVIGATION
#define KANA_L1 LT(_NAVIGATION, KC_LNG1)

// Built-in Layer-Tap for Eisu: tap = KC_LNG2, hold = _MODIFIER
#define EISU_LT LT(_MODIFIER, KC_LNG2)

// ---------- Runtime state ----------
static bool g_is_mac = false;  // updated by OS detection callback

// ---------- OS detection (requires OS_DETECTION_ENABLE in rules.mk) ----------
bool process_detected_host_os_user(os_variant_t os) {
    g_is_mac = (os == OS_MACOS || os == OS_IOS);
    return true;
}

// ---------- Keymaps ----------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_PPLS, KC_EQL,  KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_LPRN, KC_LBRC, KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_MINS,
        TAB_BELOW, KC_A,  KC_S,    KC_D,    KC_F,    KC_G,    KC_RPRN, KC_RBRC, KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_TILD, KC_BSLS, KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_ESC,
                                   KC_LGUI, KC_LALT, EISU_LT, KC_SPC,  KC_SPC,  KANA_L1, KC_ENT,  KC_BTN2
    ),


    [_NAVIGATION] = LAYOUT(
        QK_BOOT, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_PWR,
        _______, _______, _______, _______, _______, _______, _______, KC_BTN4, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, KC_BTN5, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, KC_HOME, KC_PGDN, KC_PGUP, KC_END,  _______, _______,
                                   _______, _______, _______, _______, _______, _______, _______, _______
    ),

    [_EMACS] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, KC_UP,   KC_END,  _______, _______, _______, _______, KC_PGUP, _______, _______, _______, KC_UP,   _______,
        _______, KC_HOME, _______, KC_DEL,  KC_RGHT, _______, _______, _______, KC_BSPC, _______, KC_DEL,  _______, _______, _______,
        _______, _______, _______, _______, KC_PGDN, KC_LEFT, _______, _______, KC_DOWN, _______, _______, _______, _______, _______,
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
        case TAB_BELOW:
            // macOS: behave as LCTL (modifier). Windows: behave as MO(_EMACS).
            if (g_is_mac) {
                if (record->event.pressed) register_mods(MOD_BIT(KC_LCTL));
                else                       unregister_mods(MOD_BIT(KC_LCTL));
            } else {
                if (record->event.pressed) layer_on(_EMACS);
                else                        layer_off(_EMACS);
            }
            return false;
    }
    return true;
}

// ---------- Handle modifier layer for OS-specific behavior ----------
layer_state_t layer_state_set_user(layer_state_t state) {
    // When _MODIFIER layer is active, apply OS-specific modifier
    if (IS_LAYER_ON_STATE(state, _MODIFIER)) {
        if (g_is_mac) register_mods(MOD_BIT(KC_LGUI));
        else          register_mods(MOD_BIT(KC_LCTL));
    } else {
        // Clean up modifiers when leaving _MODIFIER layer
        if (g_is_mac) unregister_mods(MOD_BIT(KC_LGUI));
        else          unregister_mods(MOD_BIT(KC_LCTL));
    }
    return state;
}
