package com.cping.jo.service

import android.content.SharedPreferences
import com.cping.jo.utils.Constants
import io.github.jan.supabase.SupabaseClient
import io.github.jan.supabase.auth.Auth
import io.github.jan.supabase.createSupabaseClient
import io.github.jan.supabase.functions.Functions
import io.github.jan.supabase.postgrest.Postgrest
import io.github.jan.supabase.realtime.Realtime
import io.github.jan.supabase.storage.Storage
import io.ktor.client.HttpClient
import io.ktor.client.engine.cio.CIO
import kotlin.time.Duration.Companion.seconds

object SupabaseClientProvider {

    private var client: SupabaseClient? = null

    fun initialize(sharedPreferences: SharedPreferences): SupabaseClient {

        val httpClient = HttpClient(CIO) {
            install(io.ktor.client.plugins.DefaultRequest) {
                val token = sharedPreferences.getString(Constants.PREF_ACCESS_TOKEN, null)
                if (!token.isNullOrBlank()) {
                    headers.append("Authorization", "Bearer $token")
                }
            }
        }

        client = createSupabaseClient(
            supabaseUrl = Constants.SUPABASE_URL,
            supabaseKey = Constants.SUPABASE_KEY,
        ) {
            httpEngine = httpClient.engine

            install(Auth)
            install(Postgrest)
            install(Storage) { transferTimeout = 120.seconds }
            install(Realtime) { reconnectDelay = 2.seconds }
            install(Functions)
        }

        return client!!
    }

    fun get(): SupabaseClient {
        return client
            ?: throw IllegalStateException("SupabaseClient not initialized. Call initialize() first.")
    }

    fun reset() {
        client = null
    }
}
