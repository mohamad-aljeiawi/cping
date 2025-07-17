package com.cping.jo.navigation.graphs

import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.runtime.Composable
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import androidx.navigation.compose.rememberNavController
import com.cping.jo.screens.main.MainScreen
import com.cping.jo.screens.main.MainViewModel


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainGraph(
    rootNavController: NavHostController,
    navController: NavHostController = rememberNavController()
) {
    val viewModel = hiltViewModel<MainViewModel>()
    MainScreen(
        rootNavController = rootNavController,
        viewModel = viewModel
    )
}