package com.cping.jo.screens.auth.login

import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import androidx.core.content.edit
import androidx.core.net.toUri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.cping.jo.MainActivity
import com.cping.jo.model.LoginOtpObject
import com.cping.jo.repository.AppRepository
import com.cping.jo.utils.APIResponse
import com.cping.jo.utils.Constants
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class LoginViewModel @Inject constructor(
    private val appRepository: AppRepository,
    private val sharedPreferences: SharedPreferences,
    @ApplicationContext private val context: Context,
) : ViewModel() {

    private val _loginState = MutableStateFlow<APIResponse<LoginOtpObject>>(APIResponse.Idle())
    val loginState: StateFlow<APIResponse<LoginOtpObject>> = _loginState.asStateFlow()

    fun verifyOtp(otp: String) {
        viewModelScope.launch {
            _loginState.value = APIResponse.Loading()

            val otpResult = appRepository.verifyOtp(otp)
            if (otpResult !is APIResponse.Success) {
                _loginState.value = otpResult
                return@launch
            }
            val token = otpResult.data.token
            val personId = otpResult.data.personId

            val loginResult = appRepository.loginUser(token)
            if (loginResult !is APIResponse.Success) {
                _loginState.value = APIResponse.Error(Exception("Login failed"))
                return@launch
            }

            storeAccessToken(token)
            val deviceResult = appRepository.informationDeviceRegistration(personId)
            if (deviceResult !is APIResponse.Success) {
                _loginState.value = APIResponse.Error(Exception("Device registration failed"))
                storeAccessToken(null)
                return@launch
            }

            context.restartApp()
            _loginState.value = otpResult
        }
    }

    private fun storeAccessToken(token: String?) {
        sharedPreferences.edit {
            if (token != null) {
                putString(Constants.PREF_ACCESS_TOKEN, token)
            } else {
                remove(Constants.PREF_ACCESS_TOKEN)
            }
        }
    }

    fun register() {
        val intent = Intent(Intent.ACTION_VIEW).apply {
            data = Constants.DEEP_LINK_TELEGRAM_BOT.toUri()
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        context.startActivity(intent)
    }

    fun Context.restartApp() {
        val intent = Intent(this, MainActivity::class.java).apply {
            addFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK or
                        Intent.FLAG_ACTIVITY_CLEAR_TASK or
                        Intent.FLAG_ACTIVITY_CLEAR_TOP
            )
        }
        startActivity(intent)
    }
}
