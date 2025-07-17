package com.cping.jo.navigation

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.cping.jo.utils.Constants
import com.cping.jo.utils.SharedPrefManager
import dagger.hilt.android.lifecycle.HiltViewModel
import io.github.jan.supabase.SupabaseClient
import io.github.jan.supabase.auth.auth
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.datetime.Clock
import javax.inject.Inject

@HiltViewModel
class RootViewModel @Inject constructor(
    private val sharedPrefManager: SharedPrefManager,
    private val supabaseClient: SupabaseClient,
) : ViewModel() {

    var isLogin by mutableStateOf<Boolean?>(null)
        private set

    init {
        viewModelScope.launch {
            delay(500)
            val token = sharedPrefManager.getData(Constants.PREF_ACCESS_TOKEN)
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
