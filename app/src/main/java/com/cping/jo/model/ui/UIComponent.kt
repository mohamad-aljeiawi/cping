package com.cping.jo.model.ui

sealed class UIComponent {
    data class SwitchChip(
        val title: String,
        val checked: Boolean,
        val onToggle: (Boolean) -> Unit
    ) : UIComponent()

    data class MultiSwitchGroup(
        val title: String,
        val chips: List<SwitchChip>
    ) : UIComponent()

    data class ChipGroup(
        val title: String,
        val options: List<String>,
        val selectedIndex: Int,
        val onSelect: (Int) -> Unit
    ) : UIComponent()

    data class Slider(
        val title: String,
        val min: Float,
        val max: Float,
        val value: Float,
        val onValueChange: (Float) -> Unit
    ) : UIComponent()

    data class ButtonClick(
        val title: String,
        val onClick: () -> Unit
    ) : UIComponent()


}
