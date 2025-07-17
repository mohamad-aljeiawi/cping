package com.cping.jo.utils

import android.content.Context
import android.content.SharedPreferences
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
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
        val keyAlias = Constants.KEY_ALIAS

        if (keyStore.containsAlias(keyAlias)) {
            return keyStore.getKey(keyAlias, null) as SecretKey
        }

        val keyGenerator = KeyGenerator.getInstance("AES", "AndroidKeyStore")
        val keyGenParameterSpec = KeyGenParameterSpec.Builder(
            keyAlias,
            KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
        )
            .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
            .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
            .setKeySize(256)
            .build()

        keyGenerator.init(keyGenParameterSpec)
        return keyGenerator.generateKey()
    }

    private fun getPrefs(): SharedPreferences =
        context.getSharedPreferences(Constants.PREFS_NAME, Context.MODE_PRIVATE)

    fun putData(key: String, value: String) {
        try {
            val cipher = Cipher.getInstance(Constants.AES_MODE)
            cipher.init(Cipher.ENCRYPT_MODE, getOrCreateKey())

            val iv = cipher.iv
            val encrypted = cipher.doFinal(value.toByteArray())

            val combined = iv + encrypted
            val encoded = Base64.encodeToString(combined, Base64.DEFAULT)

            getPrefs().edit { putString(key, encoded) }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun getData(key: String): String? {
        return try {
            val encoded = getPrefs().getString(key, null) ?: return null
            val combined = Base64.decode(encoded, Base64.DEFAULT)

            val ivLength = 12
            if (combined.size < ivLength) return null

            val iv = combined.copyOfRange(0, ivLength)
            val encrypted = combined.copyOfRange(ivLength, combined.size)

            val cipher = Cipher.getInstance(Constants.AES_MODE)
            cipher.init(Cipher.DECRYPT_MODE, getOrCreateKey(), GCMParameterSpec(128, iv))

            String(cipher.doFinal(encrypted))
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    fun removeData(key: String) {
        getPrefs().edit { remove(key) }
    }
}
