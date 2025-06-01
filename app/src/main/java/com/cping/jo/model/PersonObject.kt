package com.cping.jo.model


import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class PersonObject(
    @SerialName("created_at")
    val createdAt: String,
    @SerialName("first_name")
    val firstName: String,
    @SerialName("id")
    val id: Int,
    @SerialName("is_premium")
    val isPremium: Boolean,
    @SerialName("language_code")
    val languageCode: String,
    @SerialName("last_name")
    val lastName: String,
    @SerialName("telegram_id")
    val telegramId: Long,
    @SerialName("username")
    val username: String
)