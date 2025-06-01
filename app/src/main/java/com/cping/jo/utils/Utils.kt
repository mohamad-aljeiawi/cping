package com.cping.jo.utils

import com.cping.jo.model.IpInfoObject
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject

object Utils {
    fun processBuilderShell(command: String) {
        ProcessBuilder("su", "-c", command).start().waitFor()
    }

    suspend fun getIpInfo(): IpInfoObject? = withContext(Dispatchers.IO) {
        val client = OkHttpClient()

        return@withContext try {
            val ipResponse = client.newCall(
                Request.Builder()
                    .url("https://api.ipify.org?format=json")
                    .build()
            ).execute()
            val ip =
                JSONObject(ipResponse.body?.string() ?: return@withContext null).getString("ip")

            val infoResponse = client.newCall(
                Request.Builder()
                    .url("https://ipinfo.io/$ip/json")
                    .build()
            ).execute()
            val json = JSONObject(infoResponse.body?.string() ?: return@withContext null)

            IpInfoObject(
                ip = json.getString("ip"),
                city = json.getString("city"),
                region = json.getString("region"),
                country = json.getString("country"),
                loc = json.getString("loc"),
                org = json.getString("org"),
                timezone = json.getString("timezone")
            )
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    fun getDeviceInfo(): String {
        val manufacturer = android.os.Build.MANUFACTURER
        val model = android.os.Build.MODEL
        val version = android.os.Build.VERSION.RELEASE
        return "$manufacturer $model (Android $version)"
    }
}