package com.cping.jo.navigation.graphs

import androidx.compose.runtime.Composable
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.cping.jo.screens.auth.login.LoginScreen
import com.cping.jo.screens.auth.login.LoginViewModel


@Composable
fun AuthGraph(
    rootNavController: NavHostController,
    navController: NavHostController = rememberNavController()
) {
    NavHost(
        navController = navController,
        startDestination = AuthScreens.Login.route
    ) {
        composable(AuthScreens.Login.route) {
            val viewModel = hiltViewModel<LoginViewModel>()
            LoginScreen(
                rootNavController = rootNavController,
                viewModel = viewModel
            )
        }
    }
}

sealed class AuthScreens(val route: String) {
    data object Login : AuthScreens(route = "LOGIN")
}