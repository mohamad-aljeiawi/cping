package com.cping.jo.di

import com.cping.jo.utils.Constants
import com.cping.jo.utils.SharedPrefManager
import io.github.jan.supabase.SupabaseClient
import io.github.jan.supabase.auth.Auth
import io.github.jan.supabase.createSupabaseClient
import io.github.jan.supabase.functions.Functions
import io.github.jan.supabase.postgrest.Postgrest
import io.github.jan.supabase.realtime.Realtime
import io.github.jan.supabase.storage.Storage
import io.ktor.client.HttpClient
import io.ktor.client.engine.cio.CIO
import io.ktor.client.plugins.DefaultRequest
import kotlin.time.Duration.Companion.seconds

object SupabaseClientProvider {

    private var client: SupabaseClient? = null

    fun initialize(sharedPrefManager: SharedPrefManager): SupabaseClient {

        val httpClient = HttpClient(CIO) {
            install(DefaultRequest) {
                val token = sharedPrefManager.getData(Constants.PREF_ACCESS_TOKEN)
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

            install(Auth.Companion)
            install(Postgrest.Companion)
            install(Storage.Companion) { transferTimeout = 120.seconds }
            install(Realtime.Companion) { reconnectDelay = 2.seconds }
            install(Functions.Companion)
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