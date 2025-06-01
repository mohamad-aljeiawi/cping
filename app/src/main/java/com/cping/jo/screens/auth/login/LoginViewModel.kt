package com.cping.jo.screens.auth.login

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import androidx.navigation.NavHostController
import com.cping.jo.data.DataOrException
import com.cping.jo.navigation.Graph
import com.cping.jo.repository.AppRepository
import com.cping.jo.utils.Utils
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import io.github.jan.supabase.SupabaseClient
import io.github.jan.supabase.gotrue.auth
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class LoginViewModel @Inject constructor(
    private val appRepository: AppRepository,
    private val sharedPreferences: SharedPreferences,
    private val supabaseClient: SupabaseClient,
    @ApplicationContext private val context: Context
) : ViewModel() {

    private var shouldNavigate: Boolean by mutableStateOf(false)

    private val getDeviceId: String = Utils.getDeviceId()
    private val getDeviceName: String = Utils.getDeviceName()
    private val getAuthId: String = supabaseClient.auth.currentSessionOrNull()!!.user!!.id

    var textKey by mutableStateOf("")
    var createUserData: DataOrException<Boolean, Boolean, Exception, Int> by mutableStateOf(
        DataOrException(
            null,
            false,
            Exception(""),
            -1
        )
    )

    fun createUser(rootNavController: NavHostController) {
        viewModelScope.launch {
            createUserData = DataOrException(
                null,
                true,
                Exception(""),
                -1
            )
            createUserData = appRepository.createUser(getDeviceName, getAuthId, getDeviceId)
            shouldNavigate = true

            if (createUserData.exception?.localizedMessage.isNullOrEmpty()) {
                rootNavController.navigate(Graph.MAIN) {
                    popUpTo(Graph.AUTH) { inclusive = true }
                }
            }
        }
    }
}