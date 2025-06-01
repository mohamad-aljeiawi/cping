package com.cping.jo.navigation.graphs

import android.os.Build
import androidx.annotation.RequiresApi
import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.cping.jo.navigation.Graph


@RequiresApi(Build.VERSION_CODES.O)
@Composable
fun RootGraph() {
    val rootNavController = rememberNavController()
    NavHost(
        navController = rootNavController,
        startDestination = Graph.SPLASH
    ) {
        composable(Graph.SPLASH) {
            SplashGraph(rootNavController = rootNavController)
        }
        composable(Graph.AUTH) {
            AuthGraph(rootNavController = rootNavController)
        }
        composable(Graph.MAIN) {
            MainGraph(rootNavController = rootNavController)
        }
    }
}