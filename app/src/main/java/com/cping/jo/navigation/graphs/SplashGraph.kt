package com.cping.jo.navigation.graphs

import android.annotation.SuppressLint
import androidx.compose.runtime.Composable
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import com.cping.jo.navigation.RootViewModel


@SuppressLint("UnusedMaterial3ScaffoldPaddingParameter")
@Composable
fun SplashGraph(
    rootNavController: NavHostController,
    onSplashFinished: () -> Unit = {}
) {
    val viewModel = hiltViewModel<RootViewModel>()



}