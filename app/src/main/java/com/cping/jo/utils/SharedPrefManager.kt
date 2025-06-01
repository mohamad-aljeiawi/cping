package com.cping.jo.utils

import android.content.Context
import android.content.SharedPreferences
import android.util.Base64
import dagger.hilt.android.qualifiers.ApplicationContext
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import javax.inject.Inject
import androidx.core.content.edit

class SharedPrefManager @Inject constructor(@ApplicationContext private val context: Context) {

    private val keyStore = KeyStore.getInstance("AndroidKeyStore").apply { load(null) }

    private fun getOrCreateKey(): SecretKey {
        return if (keyStore.containsAlias(Constants.KEY_ALIAS)) {
            keyStore.getKey(Constants.KEY_ALIAS, null) as SecretKey
        } else {
            val keyGenerator = KeyGenerator.getInstance("AES", "AndroidKeyStore")
            keyGenerator.init(256)
            keyGenerator.generateKey()
        }
    }

    private fun generateIV(): ByteArray = ByteArray(Constants.IV_SIZE) { 0x0 }

    private fun getPrefs(): SharedPreferences =
        context.getSharedPreferences(Constants.PREFS_NAME, Context.MODE_PRIVATE)

    fun getSharedPref(): SharedPreferences {
        val prefs = getPrefs()
        val storedVersion = prefs.getInt(Constants.PREF_VERSION_KEY, -1)

        if (storedVersion < Constants.PREF_VERSION) {
            context.deleteSharedPreferences(Constants.PREFS_NAME)
            prefs.edit { clear() }
            getPrefs().edit { putInt(Constants.PREF_VERSION_KEY, Constants.PREF_VERSION) }
        }

        return getPrefs()
    }

    fun putData(key: String, value: String) {
        try {
            val cipher = Cipher.getInstance(Constants.AES_MODE)
            val iv = generateIV()
            cipher.init(Cipher.ENCRYPT_MODE, getOrCreateKey(), GCMParameterSpec(128, iv))
            val encrypted = cipher.doFinal(value.toByteArray())
            val encoded = Base64.encodeToString(encrypted, Base64.DEFAULT)
            getSharedPref().edit { putString(key, encoded) }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun getData(key: String): String? {
        return try {
            val encrypted = getSharedPref().getString(key, null) ?: return null
            val decoded = Base64.decode(encrypted, Base64.DEFAULT)
            val cipher = Cipher.getInstance(Constants.AES_MODE)
            val iv = generateIV()
            cipher.init(Cipher.DECRYPT_MODE, getOrCreateKey(), GCMParameterSpec(128, iv))
            String(cipher.doFinal(decoded))
        } catch (e: Exception) {
            null
        }
    }

    fun clearAll() {
        getSharedPref().edit { clear() }
    }
}
