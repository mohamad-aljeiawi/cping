package com.cping.jo.model.ui

data class Section(
    val id: String,
    val title: String,
    val titleHeader: String,
    val components: List<UIComponent>
)