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
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import kotlinx.coroutines.*
import java.net.URL

object Utils {
    fun processBuilderShell(command: String) {
        ProcessBuilder("su", "-c", command).start().waitFor()
    }

    fun runFileInTmp(fileName: String) {
        ProcessBuilder("su", "-c", "/data/local/tmp/$fileName &").start()
    }

    fun fileExistsInTmp(fileName: String): Boolean {
        val file = File("/data/local/tmp/$fileName")
        return file.exists()
    }

    fun downloadFileIfNotExists(
        context: Context,
        url: String,
        fileName: String,
        onDone: (Boolean) -> Unit
    ) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val tempFile = File(context.cacheDir, fileName)

                if (tempFile.exists()) {
                    onDone(true)
                    return@launch
                }

                val connection = URL(url).openConnection()
                connection.connect()

                val input = connection.getInputStream()
                val output = FileOutputStream(tempFile)

                input.use { inp ->
                    output.use { out ->
                        inp.copyTo(out)
                    }
                }

                processBuilderShell("cp ${tempFile.absolutePath} /data/local/tmp/$fileName && chmod 777 /data/local/tmp/$fileName")
                onDone(true)
            } catch (e: Exception) {
                e.printStackTrace()
                onDone(false)
            }
        }
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
        } catch (_: Exception) {
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