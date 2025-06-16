package com.cping.jo.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class OtpObject(
    @SerialName("otp_input")
    val otp: String,
    @SerialName("ip_input")
    val ip: String,
    @SerialName("region_input")
    val region: String,
    @SerialName("device_hash_input")
    val deviceHash: String
)
