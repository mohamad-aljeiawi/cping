package com.cping.jo.utils

import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import com.cping.jo.model.IpInfoObject
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import java.security.MessageDigest

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
        val manufacturer = Build.MANUFACTURER
        val model = Build.MODEL
        val version = Build.VERSION.RELEASE
        return "$manufacturer $model (Android $version)"
    }


    fun getDeviceHash(context: Context): String {
        val signature = try {
            val packageInfo = context.packageManager.getPackageInfo(
                context.packageName,
                PackageManager.GET_SIGNING_CERTIFICATES
            )
            packageInfo.signingInfo?.apkContentsSigners[0]?.toCharsString()
        } catch (e: Exception) {
            "unknown_signature"
        }

        val rawInfo = listOf(
            Build.BRAND,
            Build.DEVICE,
            Build.HARDWARE,
            Build.MANUFACTURER,
            Build.MODEL,
            Build.PRODUCT,
            signature
        ).joinToString(separator = "|")
        val digest = MessageDigest.getInstance("SHA-256")
        val hashBytes = digest.digest(rawInfo.toByteArray(Charsets.UTF_8))
        return hashBytes.joinToString("") { "%02x".format(it) }
    }
}