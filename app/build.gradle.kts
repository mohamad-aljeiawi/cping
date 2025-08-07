import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)

    // addition plugins
    //-- hilt and dagger
    alias(libs.plugins.dagger.hilt.android)
    //-- devtools ksp
    alias(libs.plugins.devtools.ksp)
    //-- kotlin serialization
    alias(libs.plugins.kotlin.serialization)
}

android {
    namespace = "com.cping.jo"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.cping.jo"
        minSdk = 28
        targetSdk = 36
        versionCode = 1
        versionName = "1.2"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    kotlin {
        compilerOptions {
            jvmTarget = JvmTarget.JVM_11
        }
    }

    buildFeatures {
        compose = true
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.ui)
    implementation(libs.androidx.ui.graphics)
    implementation(libs.androidx.ui.tooling.preview)
    implementation(libs.androidx.material3)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(platform(libs.androidx.compose.bom))
    androidTestImplementation(libs.androidx.ui.test.junit4)
    debugImplementation(libs.androidx.ui.tooling)
    debugImplementation(libs.androidx.ui.test.manifest)

    // addition modules
    //-- hilt and dagger
    implementation(libs.hilt.android)
    ksp(libs.hilt.android.compiler)

    //-- SplashScreen API
    implementation(libs.androidx.core.splashscreen)

    // Supabase BOM
    implementation(platform(libs.supabase.bom))
    implementation(libs.supabase.auth)
    implementation(libs.supabase.postgrest)
    implementation(libs.supabase.realtime)
    implementation(libs.supabase.functions)
    implementation(libs.supabase.storage)

    // ktor engine
    implementation(libs.ktor.client.core)
    implementation(libs.ktor.client.cio)
//    implementation(libs.ktor.client.android)

    // OkHttp BOM
    implementation(platform(libs.okhttp3.bom))
    implementation(libs.okhttp3.okhttp3)
    implementation(libs.okhttp3.logging)

    // Retrofit + Serialization
    implementation(libs.retrofit)
    implementation(libs.kotlinx.serialization.json)

    //-- libsu for android root permission
    implementation(libs.topjohnwu.core)
    implementation(libs.topjohnwu.service)
    implementation(libs.topjohnwu.nio)

    // navigation compose
    implementation(libs.navigation.compose)
    implementation(libs.lifecycle.viewmodel)
    implementation(libs.hilt.navigation)
    implementation(libs.kotlinx.coroutines)

    // font awesome icons
    implementation(libs.font.awesome.icons)

    //-- img loader
    implementation(libs.coil.compose)
    implementation(libs.coil.compose.network)
}


