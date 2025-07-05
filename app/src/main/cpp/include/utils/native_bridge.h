#ifndef NATIVE_BRIDGE_H
#define NATIVE_BRIDGE_H

#include <string>
#include "utils/enum.h"


inline MenuElement parseMenuElement(const std::string &str) {
    if (str == "MENU_VISUAL_ESP_SKELETON") return MenuElement::MENU_VISUAL_ESP_SKELETON;
    if (str == "MENU_VISUAL_ESP_NAME") return MenuElement::MENU_VISUAL_ESP_NAME;
    if (str == "MENU_VISUAL_ESP_HEALTH") return MenuElement::MENU_VISUAL_ESP_HEALTH;
    if (str == "MENU_VISUAL_ESP_BOX") return MenuElement::MENU_VISUAL_ESP_BOX;
    if (str == "MENU_VISUAL_ESP_DISTANCE") return MenuElement::MENU_VISUAL_ESP_DISTANCE;
    if (str == "MENU_COMBAT_IS_AIMBOT") return MenuElement::MENU_COMBAT_IS_AIMBOT;
    if (str == "MENU_COMBAT_AIMBOT_FOV") return MenuElement::MENU_COMBAT_AIMBOT_FOV;
    if (str == "MENU_COMBAT_AIMBOT_SENSITIVITY") return MenuElement::MENU_COMBAT_AIMBOT_SENSITIVITY;
    return MenuElement::UNKNOWN;
}

void handleMenuEvent(const std::string &key, bool isChecked, float sliderValue) {
    MenuElement element = parseMenuElement(key);

    switch (element) {
        case MenuElement::MENU_VISUAL_ESP_SKELETON:

            break;
        case MenuElement::MENU_VISUAL_ESP_NAME:

            break;

        case MenuElement::MENU_VISUAL_ESP_HEALTH:

            break;
        case MenuElement::MENU_VISUAL_ESP_BOX:

            break;
        case MenuElement::MENU_VISUAL_ESP_DISTANCE:

            break;
        case MenuElement::MENU_COMBAT_IS_AIMBOT:

            break;
        case MenuElement::MENU_COMBAT_AIMBOT_FOV:

            break;
        case MenuElement::MENU_COMBAT_AIMBOT_SENSITIVITY:

            break;
        default:
            break;
    }
}

#endif //NATIVE_BRIDGE_H
