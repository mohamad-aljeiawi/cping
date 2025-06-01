package com.cping.jo.navigation.graphs

import android.os.Build
import androidx.annotation.RequiresApi
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material3.BottomAppBar
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.cping.jo.R
//import com.cping.jo.components.DialogComp
//import com.cping.jo.screens.main.MainViewModel
//import com.cping.jo.screens.main.home.HomeScreen
//import com.cping.jo.screens.main.setting.SettingScreen
import compose.icons.FontAwesomeIcons
import compose.icons.fontawesomeicons.Solid
import compose.icons.fontawesomeicons.solid.Star
import io.ktor.util.toUpperCasePreservingASCIIRules


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainGraph(
    rootNavController: NavHostController,
    navController: NavHostController = rememberNavController()
) {
//    val viewModel = hiltViewModel<MainViewModel>()
//    Scaffold(
//        topBar = {
//            TopAppBar(
//                modifier = Modifier.padding(horizontal = 14.dp),
//                title = {
//                    Row(
//                        horizontalArrangement = Arrangement.spacedBy(5.dp),
//                        verticalAlignment = Alignment.CenterVertically
//                    ) {
//                        Text(
//                            text = "${viewModel.userByDeviceIdData.data?.subscriptionName}".toUpperCasePreservingASCIIRules(),
//                            color = when (viewModel.userByDeviceIdData.data?.subscriptionName) {
//                                "Gold" -> Color(0xFFFFD700)
//                                "Silver" -> Color(0xFFC0C0C0)
//                                "Bronze" -> Color(0xFFCD7F32)
//                                else -> Color.Transparent
//                            },
//                            fontWeight = FontWeight.ExtraBold,
//                            style = MaterialTheme.typography.headlineSmall
//                        )
//                        Text(
//                            modifier = Modifier.fillMaxWidth(),
//                            text = "${viewModel.userByDeviceIdData.data?.subscriptionRemainingTime}",
//                            color = MaterialTheme.colorScheme.onBackground.copy(alpha = 0.8f),
//                            fontWeight = FontWeight.Bold,
//                            style = MaterialTheme.typography.titleMedium
//                        )
//                    }
//                },
//                navigationIcon = {
//                    Icon(
//                        modifier = Modifier.size(20.dp),
//                        imageVector = FontAwesomeIcons.Solid.Star,
//                        contentDescription = "Star",
//                        tint = when (viewModel.userByDeviceIdData.data?.subscriptionName) {
//                            "Gold" -> Color(0xFFFFD700)
//                            "Silver" -> Color(0xFFC0C0C0)
//                            "Bronze" -> Color(0xFFCD7F32)
//                            else -> Color.Transparent
//                        }
//                    )
//                },
//                actions = {
//                    SelectionContainer {
//                        Text(
//                            text = "${viewModel.userByDeviceIdData.data?.id}",
//                            color = MaterialTheme.colorScheme.onBackground.copy(alpha = 0.8f),
//                            fontWeight = FontWeight.Bold,
//                            style = MaterialTheme.typography.titleMedium
//                        )
//                    }
//                }
//            )
//        },
//        bottomBar = {
//            BottomAppBar {
//                NavigationBar(
//                    containerColor = Color.Transparent
//                ) {
//                    viewModel.itemsTopBar.forEachIndexed { index, item ->
//                        NavigationBarItem(
//                            icon = {
//                                Icon(
//                                    imageVector = item.icon!!,
//                                    contentDescription = item.label
//                                )
//                            },
//                            label = { Text(text = item.label!!) },
//                            selected = viewModel.selectedItemTopBar == index,
//                            onClick = {
//                                navController.navigate(item.route!!) {
//                                    popUpTo(viewModel.itemsTopBar[viewModel.selectedItemTopBar].route!!) {
//                                        inclusive = true
//                                    }
//                                }
//                                viewModel.selectedItemTopBar = index
//                            }
//                        )
//                    }
//                }
//            }
//        }
//    ) {
//        Box(
//            modifier = Modifier
//                .fillMaxSize()
//                .padding(it)
//        ) {
//
//            if (!viewModel.userByDeviceIdData.exception?.localizedMessage.isNullOrEmpty()) {
//                DialogComp(
//                    title = stringResource(id = R.string.text_status_code_error),
//                    description = "${stringResource(id = R.string.text_error)}:\n ${
//                        viewModel.userByDeviceIdData.exception?.localizedMessage?.substring(
//                            0,
//                            50
//                        )
//                    }...",
//                )
//            }
//
//            if (!(!viewModel.downloadFilesData.loading!! && !viewModel.userByDeviceIdData.loading!!)) {
//                CircularProgressIndicator(
//                    modifier = Modifier.align(Alignment.Center)
//                )
//            }
//
//            NavHost(
//                navController = navController,
//                startDestination = MainScreens.Home.route
//            ) {
//                composable(MainScreens.Home.route) {
//                    if (!viewModel.userByDeviceIdData.loading!! && !viewModel.downloadFilesData.loading!!) {
//                        HomeScreen(rootNavController, navController, viewModel)
//                    }
//                }
//                composable(MainScreens.Settings.route) {
//                    if (!viewModel.userByDeviceIdData.loading!! && !viewModel.downloadFilesData.loading!!) {
//                        SettingScreen(rootNavController, navController, viewModel)
//                    }
//                }
//            }
//        }
//    }
}

sealed class MainScreens(val route: String) {
    data object Home : MainScreens(route = "HOME")
    data object Settings : MainScreens(route = "SETTINGS")
}