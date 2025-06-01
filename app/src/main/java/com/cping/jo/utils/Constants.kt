package com.cping.jo.utils


object Constants {

    //- supabase api functions rpc
    const val SUPABASE_URL = "https://supabase.api.cping.nuzomix.com"
    const val SUPABASE_KEY =
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZS1kZW1vIiwicm9sZSI6ImFub24iLCJleHAiOjE5ODM4MTI5OTZ9.tymnzlYM_oh8uI1JZcsaA2Dgm7fddbjCIHhQIA653UA"
    const val SUPABASE_SERVICE_FUN_VERIFY_OTP = "fun_verify_otp"
    const val SUPABASE_TABLE_IP_INFO = "ipinfos"
    const val SUPABASE_SERVICE_FUN_INSERT_USER_DEFAULT = "fun_insert_user_default"
    const val SUPABASE_SERVICE_FUN_GET_USER_BY_DEVICE_ID = "fun_get_user_by_device_id"
    const val SUPABASE_SERVICE_STORAGE_BUCKET_PLUGIN = "plugin"

    const val DEEP_LINK_TELEGRAM_BOT = "https://t.me/cping_robot_bot?start=login"

    // - shared preferences keys
    const val PREF_ACCESS_TOKEN = "accessToken"
    const val PREF_LANG_KEY = "language"
    const val PREFS_NAME = "secure_shared_prefs"
    const val PREF_VERSION_KEY = "prefs_version"
    const val PREF_VERSION = 1
    const val KEY_ALIAS = "cping_secure_key"
    const val AES_MODE = "AES/GCM/NoPadding"
    const val IV_SIZE = 12

    const val CPING_MEMORY = "cping_memory"
    const val PACKAGE_NAME = "com.cping.jo"
}

