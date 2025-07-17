package com.cping.jo.di

import com.cping.jo.utils.SharedPrefManager
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import io.github.jan.supabase.SupabaseClient
import okhttp3.Interceptor
import okhttp3.OkHttpClient
import okhttp3.Request
import javax.inject.Singleton


@Module
@InstallIn(SingletonComponent::class)
class AppModule {


    @Provides
    @Singleton
    fun provideSupabaseClient(
        sharedPrefManager: SharedPrefManager
    ): SupabaseClient = SupabaseClientProvider.initialize(sharedPrefManager)

    @Provides
    @Singleton
    fun provideOkHttpClient(): OkHttpClient {
        return OkHttpClient.Builder()
            .addInterceptor(Interceptor { chain ->
                val original: Request = chain.request()
                val request: Request = original.newBuilder()
                    .header("accept", "application/json")
                    .method(original.method, original.body)
                    .build()
                chain.proceed(request)
            })
            .build()
    }
}