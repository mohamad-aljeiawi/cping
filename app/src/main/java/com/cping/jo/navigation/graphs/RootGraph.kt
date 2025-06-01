package com.cping.jo.navigation.graphs

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.cping.jo.navigation.Graph
import com.cping.jo.navigation.RootViewModel


@Composable
fun RootGraph(onSplashFinished: () -> Unit = {}) {
    val rootNavController = rememberNavController()
    val viewModel = hiltViewModel<RootViewModel>()

    LaunchedEffect(viewModel.isLogin) {
        if (viewModel.isLogin != null) onSplashFinished()
    }

    if (viewModel.isLogin != null) {
        NavHost(
            navController = rootNavController,
            startDestination = if (viewModel.isLogin == true) Graph.MAIN else Graph.AUTH
        ) {
            composable(Graph.AUTH) {
                AuthGraph(rootNavController = rootNavController)
            }
            composable(Graph.MAIN) {
                MainGraph(rootNavController = rootNavController)
            }
        }
    }
}