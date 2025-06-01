package com.cping.jo.navigation

import android.content.SharedPreferences
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.cping.jo.utils.Constants
import dagger.hilt.android.lifecycle.HiltViewModel
import io.github.jan.supabase.SupabaseClient
import io.github.jan.supabase.auth.auth
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.datetime.Clock
import javax.inject.Inject

@HiltViewModel
class RootViewModel @Inject constructor(
    private val sharedPreferences: SharedPreferences,
    private val supabaseClient: SupabaseClient,
) : ViewModel() {

    var isLogin by mutableStateOf<Boolean?>(null)
        private set

    init {
        viewModelScope.launch {
            delay(500)
            val token = sharedPreferences.getString(Constants.PREF_ACCESS_TOKEN, null)
            if (token.isNullOrEmpty()) {
                isLogin = false
                return@launch
            }
            val session = supabaseClient.auth.sessionManager.loadSession()
            val isValid = session?.expiresAt?.let { it > Clock.System.now() } == true
            isLogin = isValid
        }
    }
}
