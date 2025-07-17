package com.cping.jo.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class SubscriptionObject(
    @SerialName("created_at")
    val createdAt: String,
    @SerialName("status")
    val status: String,
    @SerialName("person_id")
    val personId: Int,
    @SerialName("seller_id")
    val sellerId: Boolean,
    @SerialName("plan_id")
    val planId: String,
    @SerialName("id")
    val id: String
)