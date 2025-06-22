package com.cping.jo.screens.menu

import androidx.compose.runtime.*
import androidx.compose.ui.res.stringResource
import com.cping.jo.enums.MenuElement
import com.cping.jo.model.ui.Section
import com.cping.jo.model.ui.UIComponent
import com.cping.jo.utils.SharedPrefManager
import com.cping.jo.R

@Composable
fun switchChipFromPref(
    title: String,
    key: String,
    defaultValue: Boolean = false,
    sharedPrefManager: SharedPrefManager?,
    onToggleExtra: ((Boolean) -> Unit)? = null
): UIComponent.SwitchChip {
    var state by remember {
        mutableStateOf(
            sharedPrefManager?.getData(key)?.toBooleanStrictOrNull() ?: defaultValue
        )
    }

    return UIComponent.SwitchChip(
        title = title, checked = state, onToggle = { newValue ->
            state = newValue
            sharedPrefManager?.putData(key, newValue.toString())
            onToggleExtra?.invoke(newValue)
        })
}

@Composable
fun sliderFromPref(
    title: String,
    key: String,
    defaultValue: Float,
    min: Float,
    max: Float,
    steps: Int,
    sharedPrefManager: SharedPrefManager?,
    onChangeExtra: ((Float) -> Unit)? = null
): UIComponent.Slider {
    var state by remember {
        mutableFloatStateOf(sharedPrefManager?.getData(key)?.toFloatOrNull() ?: defaultValue)
    }

    return UIComponent.Slider(
        title = title,
        min = min,
        max = max,
        value = state,
        steps = steps,
        onValueChange = { newValue ->
            state = newValue
            sharedPrefManager?.putData(key, newValue.toString())
            onChangeExtra?.invoke(newValue)
        })
}

@Composable
fun chipGroupFromPref(
    title: String,
    key: String,
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

    return UIComponent.ChipGroup(
        title = title, options = options, selectedIndex = selectedIndex, onSelect = { index ->
            selectedIndex = index
            sharedPrefManager?.putData(key, index.toString())
            onSelectExtra?.invoke(index)
        })
}

@Composable
fun multiSwitchGroupFromPref(
    title: String,
    keysWithLabels: List<Pair<String, String>>,
    defaultToggles: Map<String, Boolean> = emptyMap(),
    sharedPrefManager: SharedPrefManager?,
    onAnyToggle: ((key: String, value: Boolean) -> Unit)? = null
): UIComponent.MultiSwitchGroup {
    val chips = keysWithLabels.map { (key, label) ->
        val defaultValue = defaultToggles[key] == true
        switchChipFromPref(
            title = label,
            key = key,
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
    return listOf(
        Section(
            id = MenuElement.MENU_SECTION_VISUAL.name,
            title = stringResource(R.string.menu_visual_title),
            titleHeader = stringResource(R.string.menu_visual_title_header),
            components = listOf(
                multiSwitchGroupFromPref(
                    title = stringResource(R.string.menu_visual_esp_group_title),
                    keysWithLabels = listOf(
                        MenuElement.MENU_VISUAL_ESP_SKELETON.name to stringResource(R.string.menu_visual_esp_skeleton),
                        MenuElement.MENU_VISUAL_ESP_NAME.name to stringResource(R.string.menu_visual_esp_name),
                        MenuElement.MENU_VISUAL_ESP_HEALTH.name to stringResource(R.string.menu_visual_esp_health),
                        MenuElement.MENU_VISUAL_ESP_BOX.name to stringResource(R.string.menu_visual_esp_box)
                    ),
                    defaultToggles = mapOf(
                        MenuElement.MENU_VISUAL_ESP_SKELETON.name to true,
                        MenuElement.MENU_VISUAL_ESP_NAME.name to true,
                        MenuElement.MENU_VISUAL_ESP_HEALTH.name to true,
                        MenuElement.MENU_VISUAL_ESP_BOX.name to false
                    ),
                    sharedPrefManager = sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_visual_esp_distance),
                    MenuElement.MENU_VISUAL_ESP_DISTANCE.name, 360f, 50f, 400f, 10, sharedPrefManager
                )
            )
        ), Section(
            id = MenuElement.MENU_SECTION_COMBAT.name,
            title = stringResource(R.string.menu_combat_title),
            titleHeader = stringResource(R.string.menu_combat_title_header),
            components = listOf(
                switchChipFromPref(
                    stringResource(R.string.menu_combat_is_aimbot),
                    MenuElement.MENU_COMBAT_IS_AIMBOT.name,
                    defaultValue = false,
                    sharedPrefManager = sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_fov),
                    MenuElement.MENU_COMBAT_AIMBOT_FOV.name, 100f, 50f, 300f, 10, sharedPrefManager
                ), sliderFromPref(
                    stringResource(R.string.menu_combat_sensitivity),
                    MenuElement.MENU_COMBAT_AIMBOT_SENSITIVITY.name, 7f, 0.5f, 16f, 0, sharedPrefManager
                )
            )
        ), Section(
            id = MenuElement.MENU_SECTION_SETTING.name,
            title = stringResource(R.string.menu_setting_title),
            titleHeader = stringResource(R.string.menu_setting_title_header),
            components = listOf()
        )
    )
}
