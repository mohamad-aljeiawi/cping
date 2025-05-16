from telegram import Update
from telegram.ext import ContextTypes
from telegram.constants import ParseMode
from utils.translations import trans
from utils.utils import is_registered


async def start_logic(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user = update.message.from_user
    locale = user.language_code

    if not user.username:
        await update.message.reply_text(
            trans(
                locale,
                "You must set a Telegram username before using this service\\.\nPlease go to your Telegram settings, create a username, and try again\\.",
            ),
            parse_mode=ParseMode.MARKDOWN_V2,
        )
        return

    if not is_registered(user.id):
        await update.message.reply_text(
            trans(
                locale,
                "Welcome to our service\\!\nTo get started, please use /login to register your account\\.",
            ),
            parse_mode=ParseMode.MARKDOWN_V2,
        )
        return

    await update.message.reply_text(
        trans(
            locale,
            "Welcome back\\!\nYou can use the following commands:\n/dashboard \\- View your account information\n/login \\- Login to your account generate OTP\n/help \\- Get support and assistance",
        ),
        parse_mode=ParseMode.MARKDOWN_V2,
    )
