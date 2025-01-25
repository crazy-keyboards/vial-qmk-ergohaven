#include QMK_KEYBOARD_H
#include "ergohaven.h"
#include "print.h"

#define COMBO_TIMEOUT 175
#define BASE_KEY_1_ROW 4
#define BASE_KEY_1_COL 0
#define BASE_KEY_2_ROW 4
#define BASE_KEY_2_COL 2

typedef struct {
    uint8_t layer;
    uint8_t key_count;
    uint8_t positions[2][2];
} PhysicalCombo;

static struct {
    bool pressed;
    uint32_t timestamp;
} base_keys[2] = {{false, 0}, {false, 0}};

static bool combo_active = false; // Флаг активного комбо

const PhysicalCombo layer_combos[] = {
    {0, 2, {{4,0}, {1,1}}},
    {1, 2, {{4,0}, {1,2}}},
    {2, 2, {{4,0}, {2,1}}},
    {3, 2, {{4,0}, {2,2}}},
    {4, 2, {{4,0}, {3,1}}},
    {5, 2, {{4,0}, {3,2}}},
    {6, 2, {{4,0}, {4,1}}},
    {7, 2, {{4,0}, {4,2}}},
    {8, 2, {{4,2}, {1,0}}},
    {9, 2, {{4,2}, {1,1}}},
    {10, 2, {{4,2}, {2,0}}},
    {11, 2, {{4,2}, {2,1}}},
    {12, 2, {{4,2}, {3,0}}},
    {13, 2, {{4,2}, {3,1}}},
    {14, 2, {{4,2}, {4,0}}},
    {15, 2, {{4,2}, {4,1}}}
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT(
                        KC_MUTE,
      KC_7, KC_8,       KC_9,
      KC_4, KC_5,       KC_6,
      KC_1, KC_2,       KC_3,
      KC_0, KC_ENT, KC_ENT
  ),
  [1] = LAYOUT(
                        _______,
      KC_HOME, KC_INS,  KC_END,
      PREVWRD, KC_UP,   NEXTWRD,
      KC_LEFT, KC_DOWN, KC_RIGHT,
      KC_DEL,  _______, _______
  ),
  [2] = LAYOUT(
                        KC_BTN3,
      C(KC_X), C(KC_C), C(KC_V),
      KC_BTN1, KC_MS_U, KC_BTN2,
      KC_MS_L, KC_MS_D, KC_MS_R,
      KC_PSCR, _______, _______
  ),
  [3] = LAYOUT(
                        KC_MUTE,
      KC_BRID, KC_CPNL, KC_BRIU,
      KC_MYCM, KC_WSCH, KC_MAIL,
      KC_MPRV, KC_MPLY, KC_MNXT,
      KC_CALC, _______, _______
  ),
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
  [0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
  [1] = { ENCODER_CCW_CW(KC_PGDN, KC_PGUP) },
  [2] = { ENCODER_CCW_CW(KC_WH_D, KC_WH_U) },
  [3] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    const uint8_t row = record->event.key.row;
    const uint8_t col = record->event.key.col;

    // Обработка базовых клавиш
    if (row == BASE_KEY_1_ROW && col == BASE_KEY_1_COL) {

        uint8_t key_num = 0;

        if (record->event.pressed) {
            base_keys[key_num].pressed = true;
            base_keys[key_num].timestamp = timer_read();
        } else {
            // Разрешаем тап если не было комбо
            if (!combo_active && timer_elapsed(base_keys[key_num].timestamp) < COMBO_TIMEOUT) {
                tap_code(keycode);
            }
            base_keys[key_num].pressed = false;
        }
        return false; // Всегда блокируем оригинальное поведение
    }

    // Обработка базовых клавиш
    if (row == BASE_KEY_2_ROW && col == BASE_KEY_2_COL) {

        uint8_t key_num = 1;

        if (record->event.pressed) {
            base_keys[key_num].pressed = true;
            base_keys[key_num].timestamp = timer_read();
        } else {
            // Разрешаем тап если не было комбо
            if (!combo_active && timer_elapsed(base_keys[key_num].timestamp) < COMBO_TIMEOUT) {
                tap_code(keycode);
            }
            base_keys[key_num].pressed = false;
        }
        return false; // Всегда блокируем оригинальное поведение
    }

    // Блокировка клавиш в активном комбо
    if (combo_active) {
        for (uint8_t i = 0; i < sizeof(layer_combos)/sizeof(layer_combos[0]); i++) {
            for (uint8_t j = 0; j < layer_combos[i].key_count; j++) {
                if (row == layer_combos[i].positions[j][0] &&
                    col == layer_combos[i].positions[j][1]) {
                    return false; // Блокируем клавиши комбо
                }
            }
        }
    }

    return true;
}

void matrix_scan_user(void) {
    static uint8_t active_layer = 0;
    bool any_combo_active = false;

    for (uint8_t i = 0; i < sizeof(layer_combos)/sizeof(layer_combos[0]); i++) {
        PhysicalCombo combo = layer_combos[i];
        bool current_combo_active = true;

        for (uint8_t j = 0; j < combo.key_count; j++) {
            uint8_t r = combo.positions[j][0];
            uint8_t c = combo.positions[j][1];
            bool key_pressed = false;

            if (r == BASE_KEY_1_ROW && c == BASE_KEY_1_COL) {
                key_pressed = base_keys[0].pressed;
            } else if (r == BASE_KEY_2_ROW && c == BASE_KEY_2_COL) {
                key_pressed = base_keys[1].pressed;
            } else {
                key_pressed = matrix_is_on(r, c);
            }

            if (!key_pressed) {
                current_combo_active = false;
                break;
            }
        }

        if (current_combo_active) {
            any_combo_active = true;
            if (active_layer != combo.layer) {
                layer_off(active_layer);
                layer_on(combo.layer);
                active_layer = combo.layer;
            }
            break;
        }
    }

    // Обновляем глобальный флаг комбо
    combo_active = any_combo_active;

 }
