package com.cping.jo.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class LoginOtpObject(
    @SerialName("person_id")
    val personId: Int,
    @SerialName("token")
    val token: String
)