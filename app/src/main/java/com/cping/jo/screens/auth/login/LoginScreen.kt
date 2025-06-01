package com.cping.jo.screens.auth.login

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.CornerSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowForward
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Divider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import com.cping.jo.R
import com.cping.jo.components.DialogComp
import com.cping.jo.components.TextFieldComp
import com.cping.jo.components.TypewriterAnimationComp
import com.cping.jo.ui.theme.CostumeTheme

@Composable
fun LoginScreen(
    rootNavController: NavHostController,
    navController: NavHostController,
    viewModel: LoginViewModel = hiltViewModel()
) {
    val typeWriterTexts = listOf(
        "Seamless",
        "Fast usage",
        "Security",
        "Touch emulator"
    )

    LayoutLogin(
        textKey = viewModel.textKey,
        onKey = {
            viewModel.textKey = it
        },
        onLogin = {
            if (viewModel.textKey.isNotEmpty()) {
                viewModel.createUser(rootNavController)
            }
        },
        onJoinFree = {
            viewModel.createUser(rootNavController)
        },
        typeWriterTexts = typeWriterTexts,
        loading = viewModel.createUserData.loading!!,
        error = viewModel.createUserData.exception?.localizedMessage
    )
}

@Composable
fun LayoutLogin(
    textKey: String? = null,
    onKey: (String) -> Unit = {},
    onLogin: () -> Unit = {},
    onJoinFree: () -> Unit = {},
    typeWriterTexts: List<String> = emptyList(),
    loading: Boolean = false,
    error: String? = null,
) {
    Box(
        modifier = Modifier
            .background(color = MaterialTheme.colorScheme.background)
            .fillMaxSize()
    ) {

        if (!error.isNullOrEmpty()) {
            DialogComp(
                title = stringResource(id = R.string.text_status_code_error),
                description = "${stringResource(id = R.string.text_error)}:\n ${
                    error.substring(
                        0,
                        50
                    )
                }...",
            )
            return@Box
        }

        Column(
            modifier = Modifier
                .background(color = MaterialTheme.colorScheme.background)
                .fillMaxSize()
                .align(Alignment.TopCenter)
                .padding(top = 200.dp),
            verticalArrangement = Arrangement.Top,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            TypewriterAnimationComp(
                parts = typeWriterTexts,
                textStyle = MaterialTheme.typography.headlineMedium,
                color = MaterialTheme.colorScheme.onBackground,
                baseText = "Amazing Features",
                highlightText = ""
            )
        }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .background(
                    shape = RoundedCornerShape(
                        topStart = MaterialTheme.shapes.extraLarge.topStart,
                        topEnd = MaterialTheme.shapes.extraLarge.topEnd,
                        bottomStart = CornerSize(0),
                        bottomEnd = CornerSize(0)
                    ),
                    color = MaterialTheme.colorScheme.secondaryContainer
                )
                .padding(
                    start = 20.dp,
                    end = 20.dp,
                    top = 20.dp,
                    bottom = 20.dp + (32.dp * 1.5f)
                )
                .align(Alignment.BottomCenter),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(15.dp),
        ) {

            if (loading) {
                CircularProgressIndicator(
                    modifier = Modifier
                        .padding(20.dp)
                )
                return@Box
            }

            TextFieldComp(
                imeAction = ImeAction.Next,
                keyboardType = KeyboardType.Email,
                text = "Key",
                placeholder = "cpingAKFSSFJNSF...",
                value = textKey.toString()
            ) { textKeyInput ->

                onKey(textKeyInput)
            }

            Button(
                modifier = Modifier
                    .fillMaxWidth(),
                shape = MaterialTheme.shapes.medium,
                onClick = {
                    onLogin()
                }
            ) {
                Text(
                    style = MaterialTheme.typography.bodyLarge,
                    text = "Login",
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onPrimary
                )
            }

            Divider(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 20.dp),
                thickness = 1.dp,
                color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.12f)
            )

            OutlinedButton(
                modifier = Modifier.fillMaxWidth(),
                shape = MaterialTheme.shapes.medium,
                onClick = {
                    onJoinFree()
                }
            ) {
                Box(
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text(
                        modifier = Modifier.align(Alignment.Center),
                        style = MaterialTheme.typography.bodyLarge,
                        text = "Join For Free",
                        color = MaterialTheme.colorScheme.primary,
                        fontWeight = FontWeight.Bold
                    )
                    Icon(
                        modifier = Modifier
                            .background(
                                shape = MaterialTheme.shapes.small,
                                color = MaterialTheme.colorScheme.primary
                            )
                            .padding(2.dp)
                            .align(Alignment.CenterEnd),
                        imageVector = Icons.Default.ArrowForward, contentDescription = "",
                        tint = MaterialTheme.colorScheme.onPrimary
                    )
                }

            }
        }
    }
}

@Preview(
    showBackground = true
)
@Composable
private fun LoginScreenPreview() {
    CostumeTheme {
        LayoutLogin()
    }
}