# cping-app

A basic Android app with native C++ implementation support on rooted devices, integrated with Telegram bot for OTP-based authentication, and features Supabase-powered database/file storage and presenter management for viewing and running cheating game files.

---
<div align="center">

| <img src="https://github.com/user-attachments/assets/ded17b7b-1a06-40ed-9716-8761233ac3d5" width="300"/> | <img src="https://github.com/user-attachments/assets/6ee581ee-1f9a-4ce0-a1be-842895ce906a" width="300"/> |
|---|---|

</div>


https://github.com/user-attachments/assets/b1ed7ef5-233e-444a-97bc-282e926b92b8

https://github.com/user-attachments/assets/42f97710-f687-4286-8cf7-abd793e8fead







---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [Installation & Setup](#installation--setup)
- [Configuration](#configuration)
- [Running the Application](#running-the-application)
- [Testing & Verification](#testing--verification)
- [Native Memory Modules](#native-memory-modules)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## ✨ Features

- **🔐 Secure Authentication**: OTP-based login system via Telegram bot
- **📱 Cross-platform Support**: Native Android application built with Kotlin
- **☁️ Cloud Integration**: Full Supabase integration for database, authentication, and storage
- **📁 File Management**: Public storage bucket for hosting downloadable game enhancement files
- **🤖 Telegram Bot**: Python-powered bot for user authentication and management
- **🔒 Row-Level Security**: Database-level security policies for data protection

## 🛠️ Prerequisites

Before setting up the project, ensure you have the following installed:

### Required Software

- **Java Development Kit (JDK)**: Version 21 (preferably 23)
- **Android Studio**: Latest stable release
- **Python**: Version 3.10 or higher
- **Git**: For repository management

### Environment Variables

Set the following system environment variables:

| Variable | Description | Example |
|----------|-------------|---------|
| `JAVA_HOME` | Path to your JDK installation | `C:\Program Files\Java\jdk-21` |
| `ANDROID_HOME` | Path to your Android SDK | `C:\Users\YourName\AppData\Local\Android\Sdk` |

### External Services

- **Supabase Account**: For database and storage services
- **Telegram Account**: For bot creation via @BotFather

## 📁 Project Structure

```
cping-app/
├── app/                         # Android application (Kotlin)
│   ├── src/main/
│   │   ├── java/com/cping/jo/
│   │   │   └── utils/Constants.kt
│   │   └── ...
│   └── build.gradle
├── cping-bot-telegram/          # Telegram bot (Python)
│   ├── src/
│   │   └── main.py
│   ├── .env.example
│   └── requirements.txt
├── cping-memory-delta-force/    # Native memory module
├── cping-memory-pubg/           # Native memory module
├── database.sql                 # Supabase database schema
└── README.md
```

### Key Components

- **Android App**: `app/src/main/` - Main application entry point
- **Telegram Bot**: `cping-bot-telegram/src/main.py` - Authentication bot
- **Database Schema**: `database.sql` - Complete database structure and functions

## 🚀 Installation & Setup

### Step 1: Clone the Repository

```bash
git clone https://github.com/username/cping-app.git
cd cping-app
```

### Step 2: Supabase Database Setup

#### 2.1 Create Supabase Project

1. Visit [supabase.com](https://supabase.com) and create a new account
2. Click **"New Project"** and fill in the required details
3. Wait for project initialization to complete

#### 2.2 Configure Database Schema

1. Navigate to **SQL Editor** in your Supabase dashboard
2. Open the `database.sql` file from this repository
3. Copy all contents of the file

⚠️ **IMPORTANT**: Before executing the SQL script, you must update the JWT secret:

1. Go to **Project Settings** → **API Keys** → **JWT Keys**
2. Click **"Legacy JWT Secret"** → **"Reveal"** → **Copy** the key
3. In the SQL script, find the `fun_verify_otp` function
4. Replace `here_is_your_secret_key_jwt` with your copied JWT secret
5. Paste the modified script into the SQL Editor and run it

#### 2.3 Create Storage Bucket

1. Navigate to **Storage** in your Supabase dashboard
2. Click **"New bucket"**
3. Configure the bucket:
   - **Name**: `cping-files`
   - **Public bucket**: Enable (toggle to `true`)
4. Click **"Create"**

### Step 3: Android App Configuration

Edit the constants file: `app/src/main/java/com/cping/jo/utils/Constants.kt`

#### 3.1 Configure Supabase Connection

```kotlin
// Update these values:
const val SUPABASE_URL = "YOUR_SUPABASE_URL"
const val SUPABASE_KEY = "YOUR_SUPABASE_ANON_KEY"
```

To get these values:
- **SUPABASE_URL**: **Project Settings** → **API** → **URL**
- **SUPABASE_KEY**: **Project Settings** → **API Keys** → **anon public** key

#### 3.2 Configure Telegram Deep Link

```kotlin
const val DEEP_LINK_TELEGRAM_BOT = "https://t.me/your_bot_username?start=login"
```

*(You'll get the bot username in Step 4)*

### Step 4: Telegram Bot Setup

#### 4.1 Create Telegram Bot

1. Open Telegram and search for **@BotFather**
2. Start a conversation and use `/newbot` command
3. Follow the prompts to set:
   - **Bot Name**: Display name for your bot
   - **Bot Username**: Unique username (must end with "bot")
4. Copy the bot token provided (format: `123456789:ABCdefGHIjklMNOpqrsTUVwxyz`)

#### 4.2 Configure Bot Environment

1. Navigate to `cping-bot-telegram/` directory
2. Copy `.env.example` to `.env`:
   ```bash
   cp .env.example .env
   ```
3. Edit the `.env` file with your configuration:

```env
SUPABASE_URL=your_supabase_project_url
SUPABASE_KEY=your_supabase_service_role_key
TELEGRAM_TOKEN=your_telegram_bot_token
```

**Configuration Values:**

| Variable | Source | Security Level |
|----------|--------|----------------|
| `SUPABASE_URL` | Project Settings → API → URL | Public |
| `SUPABASE_KEY` | Project Settings → API Keys → **service_role** | 🔴 **CRITICAL** |
| `TELEGRAM_TOKEN` | @BotFather → Your bot token | 🔴 **CRITICAL** |

⚠️ **SECURITY WARNING**: The `service_role` key bypasses all Row Level Security (RLS) policies. Never expose this key client-side or in public repositories.

#### 4.3 Update Android App Deep Link

Copy your bot link from Telegram (e.g., `https://t.me/your_bot_username`) and update the `DEEP_LINK_TELEGRAM_BOT` constant in `Constants.kt`:

```kotlin
const val DEEP_LINK_TELEGRAM_BOT = "https://t.me/your_bot_username?start=login"
```

## ▶️ Running the Application

### Step 1: Start the Telegram Bot

Navigate to the bot directory and set up the Python environment:

```bash
cd cping-bot-telegram

# Create virtual environment
python -m venv .venv

# Activate virtual environment
# Windows (PowerShell)
.venv\Scripts\Activate.ps1
# Windows (Command Prompt)
.venv\Scripts\activate.bat
# macOS/Linux
source .venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Start the bot
python src/main.py
```

**Verification**: Open Telegram, find your bot, and send `/start`. You should receive a welcome message.

### Step 2: Launch Android Application

1. Open **Android Studio**
2. Select **"Open"** and navigate to the project directory
3. Wait for Gradle sync to complete
4. Ensure you have either:
   - A physical Android device connected via USB (with Developer Options enabled)
   - An Android emulator running
5. Click the **"Run"** button or use `Shift + F10`

## ✅ Testing & Verification

After both services are running, verify the complete workflow:

### Add Sample Data

Execute this SQL in your Supabase **SQL Editor**:

```sql
INSERT INTO "public"."games" 
("id", "name", "slug", "description", "image_url", "download_url", "created_at") 
VALUES 
('1', 'PUBG Mobile', 'pubg-mobile', 
 'Battle royale experience with enhanced shooting and survival capabilities using ESP and Aimbot features.', 
 'https://wallpapers.com/images/featured/pubg-thumbnail-yu5b3j8k3qa0ebtr.jpg', 
 'https://<YOUR-PROJECT-REF>.supabase.co/storage/v1/object/public/cping-files/cping_memory_pubg_v3', 
 '2025-07-16 14:40:16.147999'),

('2', 'Delta Force', 'delta-force', 
 'Tactical operations simulator with advanced combat features including ESP and precision aiming systems.', 
 'https://wallpapers.com/images/hd/special-forces-pnlkbrs6gf9515tx.jpg', 
 'https://<YOUR-PROJECT-REF>.supabase.co/storage/v1/object/public/cping-files/cping_memory_delta_force_v9', 
 '2025-07-16 14:43:39.357099');
```

⚠️ **Note**: Replace `<YOUR-PROJECT-REF>` with your actual Supabase project reference (visible in your Supabase dashboard URL).
Or leave it because I have uploaded the files to the databases if you want to try only do not change the file links
### Upload New Files

To add new downloadable content:

1. Navigate to **Storage** → **cping-files** in Supabase
2. Click **"Upload file"** and select your file
3. Copy the generated public URL
4. Insert a new row in the `games` table with the copied URL as `download_url`

## 🧩 Native Memory Modules

This project includes native memory manipulation components:

- `cping-memory-delta-force/` - Delta Force game enhancements
- `cping-memory-pubg/` - PUBG Mobile modifications

### Building Native Modules

For detailed instructions on building and running these components, follow the trusted guide:

**📖 [UE4 Memory Building Guide](https://github.com/mohamad-aljeiawi/ue4-memory?tab=readme-ov-file#building-and-running)**

### Deployment Process

1. Build the native modules following the guide above
2. Upload generated files to Supabase **Storage** → **cping-files**
3. Update the `games` table with the new file URLs
4. Ensure filename conventions match database entries (e.g., `cping_memory_pubg_v3`)

## 🔧 Troubleshooting

### Common Issues

#### Android Build Failures
- **Cause**: Incorrect JDK version or missing Android SDK
- **Solution**: Verify `JAVA_HOME` points to JDK 21/23 and `ANDROID_HOME` is properly configured

#### Supabase Authentication Errors (401/403)
- **Android App**: Verify `SUPABASE_URL` and `anon public` key in `Constants.kt`
- **Telegram Bot**: Confirm `SUPABASE_URL` and `service_role` key in `.env` file

#### Telegram Bot Won't Start
- **Python Version**: Ensure Python 3.10+ is installed and virtual environment is activated
- **Dependencies**: Run `pip install -r requirements.txt` in activated environment

#### Deep Link Not Working
- **Format**: Ensure format is `https://t.me/<bot_username>?start=login`
- **Bot Status**: Verify bot is active by testing `/start` command

#### Database Connection Issues
- **JWT Secret**: Confirm JWT secret was properly replaced in `fun_verify_otp` function
- **Network**: Check internet connectivity and Supabase project status

### Debug Checklist

- [ ] JDK 21+ installed with correct `JAVA_HOME`
- [ ] Android Studio with latest SDK components
- [ ] Python 3.10+ with virtual environment activated
- [ ] Supabase project created with correct API keys
- [ ] Database schema executed with proper JWT secret
- [ ] Storage bucket `cping-files` created as public
- [ ] Telegram bot created with valid token
- [ ] All configuration files updated with correct values

## 🤝 Contributing

We welcome contributions to improve cping-app! Please follow these guidelines:

### Development Workflow

1. **Fork** the repository
2. Create a **feature branch**: `git checkout -b feature/amazing-feature`
3. **Commit** your changes: `git commit -m 'Add amazing feature'`
4. **Push** to the branch: `git push origin feature/amazing-feature`
5. Open a **Pull Request**

### Code Standards

- Follow **Kotlin** coding conventions for Android code
- Use **PEP 8** style guide for Python code
- Include **clear commit messages**
- **Test** your changes thoroughly before submitting

## 📄 License

This project is licensed under the **MIT License**. See the [LICENSE](./LICENSE) file for details.

---

**⚠️ Disclaimer**: This software is for educational and research purposes. Users are responsible for complying with all applicable laws and game terms of service in their jurisdiction.

**🔐 Security Notice**: Keep all API keys and tokens secure. Never commit sensitive credentials to public repositories.
