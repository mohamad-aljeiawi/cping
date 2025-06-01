package com.cping.jo.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class DeviceInfoObject(
    @SerialName("person_id")
    val personId: Int,
    val ip: String,
    val city: String,
    val region: String,
    val country: String,
    val loc: String,
    val org: String,
    val timezone: String,
    @SerialName("device_info")
    val deviceInfo: String
)
