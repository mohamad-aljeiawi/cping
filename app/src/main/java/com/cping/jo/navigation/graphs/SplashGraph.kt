package com.cping.jo.navigation.graphs

import androidx.compose.runtime.Composable
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.cping.jo.screens.splash.SplashScreen
import com.cping.jo.screens.splash.SplashViewModel

@Composable
fun SplashGraph(
    rootNavController: NavHostController,
    navController: NavHostController = rememberNavController()
) {
    NavHost(
        navController = navController,
        startDestination = SplashScreens.Splash.route
    ) {
        composable(SplashScreens.Splash.route) {
            val viewModel = hiltViewModel<SplashViewModel>()
            SplashScreen(
                rootNavController = rootNavController,
                navController = navController,
                viewModel = viewModel
            )
        }
    }
}

sealed class SplashScreens(val route: String) {
    data object Splash : AuthScreens(route = "SPLASH")
}