package com.cping.jo.screens.main

import android.annotation.SuppressLint
import android.content.Context
import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.cping.jo.repository.AppRepository
import com.cping.jo.service.OverlayMenu
import com.cping.jo.service.OverlayView
import com.cping.jo.utils.SharedPrefManager
import com.cping.jo.utils.Utils
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import jakarta.inject.Inject
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.launch

@SuppressLint("SdCardPath")
@HiltViewModel
class MainViewModel @Inject constructor(
    private val appRepository: AppRepository,
    private val sharedPrefManager: SharedPrefManager,
    @ApplicationContext private val context: Context,
) : ViewModel() {

    init {

        viewModelScope.launch {
//            processBuilderShell("/data/local/tmp/cping_memory_pubg")
            OverlayView.showOverlay(context)
//            processBuilderShell("monkey -p com.tencent.ig -c android.intent.category.LAUNCHER 1")
            OverlayMenu.showOverlay(context)
        }
    }

    fun processBuilderShell(command: String) {
        try {
            ProcessBuilder(command).start()
            Log.i("CPING", "Server started")
        } catch (e: Exception) {
            Log.e("CPING", "Failed to start server", e)
        }
    }
}