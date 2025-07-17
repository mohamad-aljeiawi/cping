package com.cping.jo.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class GameObject(
    @SerialName("created_at")
    val createdAt: String,
    val name: String,
    val slug: String,
    val description: String,
    @SerialName("image_url")
    val imageUrl: String,
    @SerialName("download_url")
    val downloadUrl: String
)