package com.cping.jo.screens.main

import android.content.ClipboardManager
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.core.content.getSystemService
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.navigation.NavHostController
import com.cping.jo.model.LoginOtpObject
import com.cping.jo.navigation.Graph
import com.cping.jo.ui.theme.CpingTheme
import com.cping.jo.utils.APIResponse
import compose.icons.FontAwesomeIcons
import com.cping.jo.R
import com.cping.jo.screens.auth.login.LoginViewModel
import compose.icons.fontawesomeicons.Solid
import compose.icons.fontawesomeicons.solid.Clipboard

@Composable
fun LoginScreen(
    rootNavController: NavHostController,
    viewModel: LoginViewModel = hiltViewModel()
) {
    val loginState by viewModel.loginState.collectAsState()

    LaunchedEffect(loginState) {
        if (loginState is APIResponse.Success) {
            rootNavController.navigate(Graph.MAIN) {
                popUpTo(Graph.AUTH) { inclusive = true }
            }
        }
    }

    LayoutLogin(
        onLogin = { otp ->
            viewModel.verifyOtp(otp)
        },
        onRegister = {
            viewModel.register()
        },
        loading = loginState is APIResponse.Loading,
        error = when (loginState) {
            is APIResponse.Error -> (loginState as APIResponse.Error<LoginOtpObject>).error.localizedMessage?.substringBefore(
                "\n"
            )

            else -> null
        }
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LayoutLogin(
    onLogin: (String) -> Unit = {},
    onRegister: () -> Unit = {},
    loading: Boolean = false,
    error: String? = null,
) {
    val context = LocalContext.current
    val focusManager = LocalFocusManager.current
    var otpValue by remember { mutableStateOf("") }
    var showError by remember { mutableStateOf(false) }

    LaunchedEffect(error) {
        showError = error != null
    }

    Scaffold(
        modifier = Modifier.fillMaxSize(),
        containerColor = MaterialTheme.colorScheme.background
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(
                    start = 24.dp,
                    end = 24.dp,
                    top = paddingValues.calculateTopPadding(),
                    bottom = 48.dp
                ),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Spacer(modifier = Modifier.height(100.dp))

            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.fillMaxWidth()
            ) {
                Image(
                    painter = painterResource(id = R.drawable.ic_logo),
                    contentDescription = stringResource(id = R.string.app_logo_description),
                    modifier = Modifier.size(68.dp),
                    contentScale = ContentScale.Fit,
                )

                Spacer(modifier = Modifier.height(16.dp))

                Text(
                    text = stringResource(id = R.string.login_welcome),
                    style = MaterialTheme.typography.headlineLarge,
                    color = MaterialTheme.colorScheme.onBackground,
                    fontWeight = FontWeight.Bold,
                    textAlign = TextAlign.Center
                )

                Spacer(modifier = Modifier.height(8.dp))

                Text(
                    text = stringResource(id = R.string.login_instruction),
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    textAlign = TextAlign.Center
                )
            }

            Spacer(modifier = Modifier.height(50.dp))

            Column(
                modifier = Modifier.fillMaxWidth()
            ) {
                OutlinedTextField(
                    value = otpValue,
                    onValueChange = { newValue ->
                        if (newValue.all { it.isLetterOrDigit() } && newValue.length <= 6) {
                            otpValue = newValue
                            showError = false
                        }
                    },
                    modifier = Modifier.fillMaxWidth(),
                    label = { Text(stringResource(id = R.string.otp_label)) },
                    placeholder = { Text(stringResource(id = R.string.otp_placeholder)) },
                    leadingIcon = {
                        IconButton(
                            onClick = {
                                val clipboardManager = context.getSystemService<ClipboardManager>()
                                val clipData = clipboardManager?.primaryClip
                                if (clipData != null && clipData.itemCount > 0) {
                                    val pastedText = clipData.getItemAt(0).text?.toString() ?: ""
                                    val filteredText = pastedText.filter { it.isLetterOrDigit() }
                                    if (filteredText.isNotEmpty() && filteredText.length <= 8) {
                                        otpValue = filteredText
                                        showError = false
                                    }
                                }
                            }
                        ) {
                            Icon(
                                imageVector = FontAwesomeIcons.Solid.Clipboard,
                                contentDescription = stringResource(id = R.string.otp_paste),
                                tint = MaterialTheme.colorScheme.primary,
                                modifier = Modifier.size(18.dp)
                            )
                        }
                    },
                    keyboardOptions = KeyboardOptions(
                        keyboardType = KeyboardType.Text,
                        imeAction = ImeAction.Done
                    ),
                    keyboardActions = KeyboardActions(
                        onDone = {
                            if (otpValue.isNotEmpty()) {
                                focusManager.clearFocus()
                                onLogin(otpValue)
                            }
                        }
                    ),
                    isError = showError,
                    supportingText = if (showError && error != null) {
                        {
                            Text(
                                text = error,
                                color = MaterialTheme.colorScheme.error,
                                style = MaterialTheme.typography.bodySmall
                            )
                        }
                    } else null,
                    singleLine = true,
                    shape = MaterialTheme.shapes.medium
                )
            }

            Spacer(modifier = Modifier.height(10.dp))

            Button(
                onClick = {
                    if (otpValue.isNotEmpty()) {
                        focusManager.clearFocus()
                        onLogin(otpValue)
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp),
                enabled = otpValue.isNotEmpty() && !loading,
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.primary,
                    contentColor = MaterialTheme.colorScheme.onPrimary,
                    disabledContainerColor = MaterialTheme.colorScheme.surfaceVariant
                ),
                elevation = ButtonDefaults.buttonElevation(
                    defaultElevation = 2.dp,
                    pressedElevation = 4.dp,
                    disabledElevation = 0.dp
                ),
                shape = MaterialTheme.shapes.medium
            ) {
                if (loading) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(20.dp),
                        color = MaterialTheme.colorScheme.onPrimary,
                        strokeWidth = 2.dp
                    )
                    Spacer(modifier = Modifier.width(12.dp))
                }
                Text(
                    text = if (loading)
                        stringResource(id = R.string.login_loading)
                    else
                        stringResource(id = R.string.login_button),
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.SemiBold
                )
            }

            Spacer(modifier = Modifier.weight(1f))

            Row(
                horizontalArrangement = Arrangement.Center,
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .fillMaxWidth()
            ) {
                Text(
                    text = stringResource(id = R.string.register_cta),
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                TextButton(
                    onClick = { onRegister() },
                    contentPadding = PaddingValues(horizontal = 4.dp)
                ) {
                    Text(
                        text = stringResource(id = R.string.register_now),
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.primary,
                        fontWeight = FontWeight.SemiBold
                    )
                }
            }
        }
    }
}

@Preview(
    showBackground = true,
    name = "Login Screen - AR",
    locale = "ar"
)
@Composable
private fun LoginScreenPreview() {
    CpingTheme(darkTheme = true) {
        LayoutLogin()
    }
}

@Preview(
    showBackground = true,
    name = "Login Screen - Dark",
    locale = "en"
)
@Composable
private fun LoginScreenDarkPreview() {
    CpingTheme(darkTheme = true) {
        LayoutLogin()
    }
}