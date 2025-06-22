package com.cping.jo.screens.menu

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import com.cping.jo.model.ui.Section
import com.cping.jo.ui.theme.CpingTheme
import com.cping.jo.utils.SharedPrefManager
import compose.icons.FontAwesomeIcons
import compose.icons.fontawesomeicons.Solid
import compose.icons.fontawesomeicons.solid.Ban

@Composable
fun MenuScreen(
    sharedPrefManager: SharedPrefManager,
    onHideMenu: () -> Unit = {},
) {
    LayoutMenu(
        sharedPrefManager = sharedPrefManager,
        onHideMenu = { onHideMenu() }
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LayoutMenu(
    sharedPrefManager: SharedPrefManager? = null,
    onHideMenu: () -> Unit = {},
) {
    val layoutDirection = LocalLayoutDirection.current
    val allSections = buildAllMenuSections(sharedPrefManager)
    var selectedSectionIndex by remember { mutableIntStateOf(0) }
    val selectedSection = allSections[selectedSectionIndex]

    Surface(
        modifier = Modifier
            .size(width = 150.dp * 7, height = 260.dp)
            .padding(0.dp),
        shape = MaterialTheme.shapes.medium,
        color = MaterialTheme.colorScheme.background,
        border = BorderStroke(1.dp, MaterialTheme.colorScheme.primary),
    ) {
        Column(
            modifier = Modifier.fillMaxSize(),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            val primaryColor = MaterialTheme.colorScheme.primary
            Row(
                modifier = Modifier
                    .drawBehind {
                        val strokeWidth = 1 * density
                        val y = size.height - strokeWidth / 2

                        drawLine(
                            primaryColor,
                            Offset(0f, y),
                            Offset(size.width, y),
                            strokeWidth
                        )
                    }
                    .fillMaxWidth()
                    .height(40.dp)
                    .padding(8.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = selectedSection.titleHeader,
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.padding(horizontal = 10.dp)
                )

                IconButton(
                    onClick = { onHideMenu() }
                ) {
                    Icon(
                        imageVector = FontAwesomeIcons.Solid.Ban,
                        contentDescription = "Cancel",
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
            }
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
            ) {
                Column(
                    modifier = Modifier
                        .drawBehind {
                            val strokeWidth = 1 * density
                            val x = if (layoutDirection == LayoutDirection.Rtl) {
                                strokeWidth / 2
                            } else {
                                size.width - strokeWidth / 2
                            }

                            drawLine(
                                primaryColor,
                                Offset(x, 0f),
                                Offset(x, size.height),
                                strokeWidth
                            )
                        }
                        .width(120.dp)
                        .padding(vertical = 4.dp, horizontal = 8.dp)
                        .fillMaxHeight()
                ) {
                    LoginMenuSelector(
                        sections = allSections,
                        selectedIndex = selectedSectionIndex,
                        onItemSelected = { selectedSectionIndex = it }
                    )
                }
                Column(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxHeight()
                        .padding(vertical = 4.dp, horizontal = 8.dp)
                ) {
                    DynamicSection(components = selectedSection.components)
                }
            }
        }
    }
}

@Composable
private fun LoginMenuSelector(
    sections: List<Section>,
    selectedIndex: Int,
    onItemSelected: (Int) -> Unit
) {
    LazyColumn(
        modifier = Modifier.fillMaxWidth(),
        horizontalAlignment = Alignment.Start,
        verticalArrangement = Arrangement.spacedBy(2.dp)
    ) {
        items(sections.size) { index ->
            val section = sections[index]
            Button(
                modifier = Modifier.fillMaxWidth(),
                shape = MaterialTheme.shapes.small,
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (selectedIndex == index)
                        MaterialTheme.colorScheme.primary
                    else
                        MaterialTheme.colorScheme.secondaryContainer,
                    contentColor = if (selectedIndex == index)
                        MaterialTheme.colorScheme.onPrimary
                    else
                        MaterialTheme.colorScheme.onSecondaryContainer
                ),
                onClick = { onItemSelected(index) },
            ) {
                Text(
                    text = section.title,
                    style = MaterialTheme.typography.labelMedium,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    textAlign = TextAlign.Center
                )
            }
        }
    }
}


@Preview(showBackground = true, name = "Login Screen - AR", locale = "ar")
@Composable
private fun LoginScreenPreview() {
    CpingTheme(darkTheme = true) {
        LayoutMenu()
    }
}

@Preview(showBackground = true, name = "Login Screen - Dark", locale = "en")
@Composable
private fun LoginScreenDarkPreview() {
    CpingTheme(darkTheme = true) {
        LayoutMenu()
    }
}