package com.cping.jo.screens.main

import android.content.Context
import androidx.lifecycle.ViewModel
import com.cping.jo.repository.AppRepository
import com.cping.jo.service.OverlayMenu
import com.cping.jo.utils.SharedPrefManager
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import jakarta.inject.Inject

@HiltViewModel
class MainViewModel @Inject constructor(
    private val appRepository: AppRepository,
    private val sharedPrefManager: SharedPrefManager,
    @ApplicationContext private val context: Context,
) : ViewModel() {

    init {
//        OverlayView.showOverlay(context)
        OverlayMenu.showOverlay(context)
    }
}