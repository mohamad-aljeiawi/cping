package com.cping.jo.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class PlanObject(
    @SerialName("created_at")
    val createdAt: String,
    @SerialName("price")
    val price: String,
    @SerialName("feature_mask")
    val featureMask: Int,
    @SerialName("description")
    val description: Boolean,
    @SerialName("name")
    val name: String,
    @SerialName("id")
    val id: String
)