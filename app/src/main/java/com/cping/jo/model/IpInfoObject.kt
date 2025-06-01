package com.cping.jo.model

import kotlinx.serialization.Serializable

@Serializable
data class IpInfoObject(
    val ip: String,
    val city: String,
    val region: String,
    val country: String,
    val loc: String,
    val org: String,
    val timezone: String
)
