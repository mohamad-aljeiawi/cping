package com.cping.jo.model

data class UserProfileUi(
    val fullName: String,
    val username: String,
    val isPremium: Boolean,
    val currentPlan: String,
    val subscriptionStatus: String, // "active", "expired", "canceled"
    val features: List<String> = emptyList()
)