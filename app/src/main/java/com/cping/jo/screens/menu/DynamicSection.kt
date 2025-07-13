package com.cping.jo.screens.menu

import android.annotation.SuppressLint
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.cping.jo.model.ui.UIComponent
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.lazy.LazyColumn

@SuppressLint("DefaultLocale")
@Composable
fun DynamicSection(components: List<UIComponent>) {
    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .padding(horizontal = 0.dp, vertical = 0.dp),
        verticalArrangement = Arrangement.spacedBy(0.dp)
    ) {
        item {
            components.forEach { component ->
                when (component) {
                    is UIComponent.SwitchChip -> {
                        FilterChip(
                            selected = component.checked,
                            onClick = { component.onToggle(!component.checked) },
                            label = {
                                Text(
                                    text = component.title,
                                    style = MaterialTheme.typography.labelLarge
                                )
                            }
                        )
                    }

                    is UIComponent.ChipGroup -> {
                        Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                            Text(
                                text = component.title,
                                style = MaterialTheme.typography.titleSmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                                component.options.forEachIndexed { index, option ->
                                    FilterChip(
                                        selected = component.selectedIndex == index,
                                        onClick = { component.onSelect(index) },
                                        label = {
                                            Text(
                                                text = option,
                                                style = MaterialTheme.typography.labelMedium
                                            )
                                        }
                                    )
                                }
                            }
                        }
                    }

                    is UIComponent.MultiSwitchGroup -> {
                        Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                            Text(
                                text = component.title,
                                style = MaterialTheme.typography.titleSmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                                component.chips.forEach { chip ->
                                    FilterChip(
                                        selected = chip.checked,
                                        onClick = { chip.onToggle(!chip.checked) },
                                        label = {
                                            Text(
                                                text = chip.title,
                                                style = MaterialTheme.typography.labelMedium
                                            )
                                        }
                                    )
                                }
                            }
                        }
                    }

                    is UIComponent.Slider -> {
                        Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Text(
                                    text = component.title,
                                    style = MaterialTheme.typography.titleSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                                Text(
                                    text = String.format("%.3f", component.value),
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }

                            Slider(
                                value = component.value,
                                onValueChange = component.onValueChange,
                                valueRange = component.min..component.max,
                                modifier = Modifier.fillMaxWidth()
                            )
                        }
                    }

                    is UIComponent.ButtonClick -> {
                        Button(
                            onClick = component.onClick,
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Text(
                                text = component.title,
                                style = MaterialTheme.typography.labelLarge
                            )
                        }
                    }
                }

                Spacer(modifier = Modifier.height(16.dp))
            }
        }
    }
}

