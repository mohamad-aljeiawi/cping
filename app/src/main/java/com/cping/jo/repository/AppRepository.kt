package com.cping.jo.repository

import android.content.Context
import android.util.Log
import com.cping.jo.model.DeviceInfoObject
import com.cping.jo.model.IpInfoObject
import com.cping.jo.model.LoginOtpObject
import com.cping.jo.model.OtpObject
import com.cping.jo.utils.APIResponse
import com.cping.jo.utils.Constants
import com.cping.jo.utils.Utils
import dagger.hilt.android.qualifiers.ApplicationContext
import io.github.jan.supabase.SupabaseClient
import io.github.jan.supabase.auth.auth
import io.github.jan.supabase.postgrest.postgrest
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.jsonObject
import javax.inject.Inject
import io.github.jan.supabase.auth.user.UserSession
import kotlin.toString
import com.cping.jo.di.SupabaseClientProvider
import com.cping.jo.utils.SharedPrefManager

class AppRepository @Inject constructor(
    private val supabaseClient: SupabaseClient,
    @ApplicationContext private val context: Context,
    private val sharedPrefManager: SharedPrefManager
) {

    suspend fun verifyOtp(otp: String): APIResponse<LoginOtpObject> {
        try {
            val ipInfo: IpInfoObject? = Utils.getIpInfo()
            val deviceHash = Utils.getDeviceHash(context)
            val response = supabaseClient.postgrest.rpc(
                Constants.SUPABASE_SERVICE_FUN_VERIFY_OTP,
                Json.encodeToJsonElement(
                    OtpObject.serializer(),
                    OtpObject(
                        otp = otp,
                        ip = ipInfo?.ip ?: "-",
                        region = ipInfo?.region ?: "-",
                        deviceHash = deviceHash
                    )
                ).jsonObject
            ).decodeSingle<LoginOtpObject>()
            Log.d("TAG_RESPONSE", "RESPONSE LOGIN OTP: ${response.toString()}")
            return APIResponse.Success<LoginOtpObject>(response)
        } catch (error: Exception) {
            Log.d("TAG_RESPONSE", "ERROR REQUEST LOGIN OTP: ${error.toString()}")
            return APIResponse.Error<LoginOtpObject>(error)
        }
    }

    suspend fun loginUser(accessToken: String): APIResponse<Unit> {
        return try {
            val session = UserSession(
                accessToken = accessToken,
                refreshToken = "",
                providerRefreshToken = null,
                providerToken = null,
                tokenType = "bearer",
                user = null,
                expiresIn = 60 * 60 * 24 * 14
            )
            supabaseClient.auth.sessionManager.saveSession(session)

            Log.d("TAG_RESPONSE", "Session saved. Access token: $accessToken")
            APIResponse.Success(Unit)
        } catch (e: Exception) {
            Log.d("TAG_RESPONSE", "Error saving session: ${e.message}")
            APIResponse.Error(e)
        }
    }

    suspend fun informationDeviceRegistration(personId: Int): APIResponse<Unit> {
        return try {
            val supabase = SupabaseClientProvider.initialize(sharedPrefManager)

            val ipInfo: IpInfoObject? = Utils.getIpInfo()
            val deviceInfo = Utils.getDeviceInfo()
            val deviceHash = Utils.getDeviceHash(context)
            val responseInfo =
                supabase.postgrest.from(Constants.SUPABASE_TABLE_IP_INFO).insert(
                    DeviceInfoObject(
                        personId = personId,
                        ip = ipInfo?.ip ?: "-",
                        city = ipInfo?.city ?: "-",
                        region = ipInfo?.region ?: "-",
                        country = ipInfo?.country ?: "-",
                        loc = ipInfo?.loc ?: "-",
                        org = ipInfo?.org ?: "-",
                        timezone = ipInfo?.timezone ?: "-",
                        deviceInfo = deviceInfo,
                        deviceHash = deviceHash
                    )
                )
            Log.d("TAG_RESPONSE", "RESPONSE INFO: ${responseInfo.data.toString()}")
            APIResponse.Success(Unit)
        } catch (e: Exception) {
            Log.d("TAG_RESPONSE", "ERROR REQUEST INFO: ${e.toString()}")
            APIResponse.Error(e)
        }
    }
}