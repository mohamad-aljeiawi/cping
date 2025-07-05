package com.cping.jo.utils


object Constants {

    //- supabase api functions rpc
    const val SUPABASE_URL = "https://ibirujhtigdwqxezcfyi.supabase.co"
    const val SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImliaXJ1amh0aWdkd3F4ZXpjZnlpIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDcxMDUxNTIsImV4cCI6MjA2MjY4MTE1Mn0.Jr9XckLE5mgLlSpsziQUQqoJCW8Hh4eGuSSCLdOEQTE"
    const val SUPABASE_SERVICE_FUN_VERIFY_OTP = "fun_verify_otp"
    const val SUPABASE_TABLE_IP_INFO = "ipinfos"

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

