package com.cping.jo.screens.menu

import android.content.res.Resources
import androidx.compose.runtime.*
import androidx.compose.ui.res.stringResource
import com.cping.jo.enums.MenuElement
import com.cping.jo.model.ui.Section
import com.cping.jo.model.ui.UIComponent
import com.cping.jo.utils.SharedPrefManager
import com.cping.jo.R
import com.cping.jo.enums.MenuSection
import com.cping.jo.utils.NativeBridge

@Composable
fun switchChipFromPref(
    title: String,
    key: String,
    keyBridge: Int,
    defaultValue: Boolean = false,
    sharedPrefManager: SharedPrefManager?,
    onToggleExtra: ((Boolean) -> Unit)? = null
): UIComponent.SwitchChip {
    var state by remember {
        mutableStateOf(
            sharedPrefManager?.getData(key)?.toBooleanStrictOrNull() ?: defaultValue
        )
    }

    NativeBridge.dispatchMenuEvent(keyBridge, state, 0f)

    return UIComponent.SwitchChip(
        title = title, checked = state, onToggle = { newValue ->
            state = newValue
            sharedPrefManager?.putData(key, newValue.toString())
            onToggleExtra?.invoke(newValue)
            NativeBridge.dispatchMenuEvent(keyBridge, newValue, 0f)
        })
}

@Composable
fun sliderFromPref(
    title: String,
    key: String,
    keyBridge: Int,
    defaultValue: Float,
    min: Float,
    max: Float,
    sharedPrefManager: SharedPrefManager?,
    onChangeExtra: ((Float) -> Unit)? = null
): UIComponent.Slider {
    var state by remember {
        mutableFloatStateOf(sharedPrefManager?.getData(key)?.toFloatOrNull() ?: defaultValue)
    }

    NativeBridge.dispatchMenuEvent(keyBridge, false, state)

    return UIComponent.Slider(
        title = title,
        min = min,
        max = max,
        value = state,
        onValueChange = { newValue ->
            state = newValue
            sharedPrefManager?.putData(key, newValue.toString())
            onChangeExtra?.invoke(newValue)
            NativeBridge.dispatchMenuEvent(keyBridge, false, newValue)
        })
}

@Composable
fun chipGroupFromPref(
    title: String,
    key: String,
    keyBridge: Int,
    defaultOption: Int,
    options: List<String>,
    sharedPrefManager: SharedPrefManager?,
    onSelectExtra: ((Int) -> Unit)? = null
): UIComponent.ChipGroup {
    var selectedIndex by remember {
        mutableIntStateOf(
            sharedPrefManager?.getData(key)?.toIntOrNull() ?: defaultOption
        )
    }

    NativeBridge.dispatchMenuEvent(keyBridge, false, selectedIndex.toFloat())

    return UIComponent.ChipGroup(
        title = title, options = options, selectedIndex = selectedIndex, onSelect = { index ->
            selectedIndex = index
            sharedPrefManager?.putData(key, index.toString())
            onSelectExtra?.invoke(index)
            NativeBridge.dispatchMenuEvent(keyBridge, false, index.toFloat())
        })
}

@Composable
fun multiSwitchGroupFromPref(
    title: String,
    keysWithLabels: List<Pair<String, String>>,
    keyBridgeMap: Map<String, Int>,
    defaultToggles: Map<String, Boolean> = emptyMap(),
    sharedPrefManager: SharedPrefManager?,
    onAnyToggle: ((key: String, value: Boolean) -> Unit)? = null
): UIComponent.MultiSwitchGroup {
    val chips = keysWithLabels.map { (key, label) ->
        val defaultValue = defaultToggles[key] == true
        val keyBridge = keyBridgeMap[key] ?: -1

        switchChipFromPref(
            title = label,
            key = key,
            keyBridge = keyBridge,
            defaultValue = defaultValue,
            sharedPrefManager = sharedPrefManager,
            onToggleExtra = { value -> onAnyToggle?.invoke(key, value) })
    }

    return UIComponent.MultiSwitchGroup(
        title = title, chips = chips
    )
}

@Composable
fun buildAllMenuSections(sharedPrefManager: SharedPrefManager?): List<Section> {
    var screenWidth = Resources.getSystem().displayMetrics.widthPixels.toFloat()
    var screenHeight = Resources.getSystem().displayMetrics.heightPixels.toFloat()

    if (screenHeight > screenWidth) {
        val temp = screenWidth
        screenWidth = screenHeight
        screenHeight = temp
    }

    return listOf(
        Section(
            id = MenuSection.MENU_SECTION_VISUAL.name,
            title = stringResource(R.string.menu_visual_title),
            titleHeader = stringResource(R.string.menu_visual_title_header),
            components = listOf(
                multiSwitchGroupFromPref(
                    title = stringResource(R.string.menu_visual_esp_group_title),
                    keysWithLabels = listOf(
                        MenuElement.MENU_VISUAL_ESP_BOX.name to stringResource(R.string.menu_visual_esp_box),
                        MenuElement.MENU_VISUAL_ESP_HEALTH.name to stringResource(R.string.menu_visual_esp_health),
                        MenuElement.MENU_VISUAL_ESP_NAME.name to stringResource(R.string.menu_visual_esp_name),
                        MenuElement.MENU_VISUAL_ESP_MARKS.name to stringResource(R.string.menu_visual_esp_marks),
                    ),
                    keyBridgeMap = mapOf(
                        MenuElement.MENU_VISUAL_ESP_BOX.name to MenuElement.MENU_VISUAL_ESP_BOX.ordinal,
                        MenuElement.MENU_VISUAL_ESP_HEALTH.name to MenuElement.MENU_VISUAL_ESP_HEALTH.ordinal,
                        MenuElement.MENU_VISUAL_ESP_NAME.name to MenuElement.MENU_VISUAL_ESP_NAME.ordinal,
                        MenuElement.MENU_VISUAL_ESP_MARKS.name to MenuElement.MENU_VISUAL_ESP_MARKS.ordinal,
                    ),
                    defaultToggles = mapOf(
                        MenuElement.MENU_VISUAL_ESP_BOX.name to false,
                        MenuElement.MENU_VISUAL_ESP_HEALTH.name to false,
                        MenuElement.MENU_VISUAL_ESP_NAME.name to false,
                        MenuElement.MENU_VISUAL_ESP_MARKS.name to false,
                    ),
                    sharedPrefManager = sharedPrefManager
                )
            )
        ), Section(
            id = MenuSection.MENU_SECTION_COMBAT.name,
            title = stringResource(R.string.menu_combat_title),
            titleHeader = stringResource(R.string.menu_combat_title_header),
            components = listOf(
                switchChipFromPref(
                    stringResource(R.string.menu_combat_is_aim),
                    MenuElement.MENU_COMBAT_IS_AIM.name,
                    MenuElement.MENU_COMBAT_IS_AIM.ordinal,
                    defaultValue = false,
                    sharedPrefManager = sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_fov),
                    MenuElement.MENU_COMBAT_AIM_FOV.name,
                    MenuElement.MENU_COMBAT_AIM_FOV.ordinal,
                    200f,
                    100f,
                    300f,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_sensitivity_factor),
                    MenuElement.MENU_COMBAT_AIM_SENSITIVITY_FACTOR.name,
                    MenuElement.MENU_COMBAT_AIM_SENSITIVITY_FACTOR.ordinal,
                    0.004f,
                    0.0f,
                    1.0f,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_swipe_duration),
                    MenuElement.MENU_COMBAT_AIM_SWIPE_DURATION.name,
                    MenuElement.MENU_COMBAT_AIM_SWIPE_DURATION.ordinal,
                    10.0f,
                    5.0f,
                    50.0f,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_latency_drag),
                    MenuElement.MENU_COMBAT_AIM_LATENCY_DRAG.name,
                    MenuElement.MENU_COMBAT_AIM_LATENCY_DRAG.ordinal,
                    0.13f,
                    0.0f,
                    1.0f,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_touch_x),
                    MenuElement.MENU_COMBAT_AIM_TOUCH_X.name,
                    MenuElement.MENU_COMBAT_AIM_TOUCH_X.ordinal,
                    1665.0f,
                    0.0f,
                    screenWidth,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_touch_y),
                    MenuElement.MENU_COMBAT_AIM_TOUCH_Y.name,
                    MenuElement.MENU_COMBAT_AIM_TOUCH_Y.ordinal,
                    475.0f,
                    0.0f,
                    screenHeight,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_touch_radius),
                    MenuElement.MENU_COMBAT_AIM_TOUCH_RADIUS.name,
                    MenuElement.MENU_COMBAT_AIM_TOUCH_RADIUS.ordinal,
                    50.0f,
                    25.0f,
                    100.0f,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_zone_fire_x),
                    MenuElement.MENU_COMBAT_AIM_ZONE_FIRE_X.name,
                    MenuElement.MENU_COMBAT_AIM_ZONE_FIRE_X.ordinal,
                    380.0f,
                    0.0f,
                    screenWidth,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_zone_fire_y),
                    MenuElement.MENU_COMBAT_AIM_ZONE_FIRE_Y.name,
                    MenuElement.MENU_COMBAT_AIM_ZONE_FIRE_Y.ordinal,
                    180.0f,
                    0.0f,
                    screenHeight,
                    sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_aim_zone_fire_radius),
                    MenuElement.MENU_COMBAT_AIM_ZONE_FIRE_RADIUS.name,
                    MenuElement.MENU_COMBAT_AIM_ZONE_FIRE_RADIUS.ordinal,
                    150.0f,
                    25.0f,
                    200.0f,
                    sharedPrefManager
                )
            )
        )
    )
}