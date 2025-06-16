package com.cping.jo.screens.menu

import androidx.compose.runtime.*
import com.cping.jo.model.ui.Section
import com.cping.jo.model.ui.UIComponent
import com.cping.jo.utils.SharedPrefManager

@Composable
fun switchChipFromPref(
    title: String,
    key: String,
    sharedPrefManager: SharedPrefManager?,
    onToggleExtra: ((Boolean) -> Unit)? = null
): UIComponent.SwitchChip {
    var state by remember { mutableStateOf(sharedPrefManager?.getData(key)?.toBoolean() == true) }

    return UIComponent.SwitchChip(
        title = title,
        checked = state,
        onToggle = { newValue ->
            state = newValue
            sharedPrefManager?.putData(key, newValue.toString())
            onToggleExtra?.invoke(newValue)
        }
    )
}

@Composable
fun sliderFromPref(
    title: String,
    key: String,
    min: Float,
    max: Float,
    steps: Int,
    sharedPrefManager: SharedPrefManager?,
    onChangeExtra: ((Float) -> Unit)? = null
): UIComponent.Slider {
    var state by remember {
        mutableFloatStateOf(
            sharedPrefManager?.getData(key)?.toFloatOrNull() ?: max
        )
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
        }
    )
}

@Composable
fun chipGroupFromPref(
    title: String,
    key: String,
    options: List<String>,
    sharedPrefManager: SharedPrefManager?,
    onSelectExtra: ((Int) -> Unit)? = null
): UIComponent.ChipGroup {
    var selectedIndex by remember {
        mutableIntStateOf(
            sharedPrefManager?.getData(key)?.toIntOrNull() ?: 0
        )
    }

    return UIComponent.ChipGroup(
        title = title,
        options = options,
        selectedIndex = selectedIndex,
        onSelect = { index ->
            selectedIndex = index
            sharedPrefManager?.putData(key, index.toString())
            onSelectExtra?.invoke(index)
        }
    )
}

@Composable
fun multiSwitchGroupFromPref(
    title: String,
    keysWithLabels: List<Pair<String, String>>,
    sharedPrefManager: SharedPrefManager?,
    onAnyToggle: ((key: String, value: Boolean) -> Unit)? = null
): UIComponent.MultiSwitchGroup {
    val chips = keysWithLabels.map { (key, label) ->
        switchChipFromPref(
            title = label,
            key = key,
            sharedPrefManager = sharedPrefManager,
            onToggleExtra = { value -> onAnyToggle?.invoke(key, value) }
        )
    }

    return UIComponent.MultiSwitchGroup(
        title = title,
        chips = chips
    )
}

@Composable
fun buildAllMenuSections(sharedPrefManager: SharedPrefManager?): List<Section> {
    return listOf(
        Section(
            id = "visual",
            title = "Visual",
            titleHeader = "Visual Player",
            components = listOf(
                multiSwitchGroupFromPref(
                    title = "Esp",
                    keysWithLabels = listOf(
                        "skeleton_line" to "Skeleton + Line",
                        "name_color" to "Name + Team Color",
                        "health" to "Health",
                        "box" to "Box"
                    ),
                    sharedPrefManager = sharedPrefManager
                ),
                sliderFromPref("Distance ESP", "distance_esp", 50f, 300f, 0, sharedPrefManager)
            )
        ),
        Section(
            id = "combat",
            title = "Combat",
            titleHeader = "Combat Player",
            components = listOf(
                sliderFromPref("FOV", "aim_fov", 30f, 180f, 0, sharedPrefManager),
                sliderFromPref("Sensitivity", "aim_sensitivity", 0f, 100f, 0, sharedPrefManager)
            )
        ),
        Section(
            id = "setting",
            title = "Setting",
            titleHeader = "Settings",
            components = listOf(
            )
        )
    )
}
