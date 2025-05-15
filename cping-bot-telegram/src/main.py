from telegram import Update
from telegram.ext import (
    ApplicationBuilder,
    ContextTypes,
    filters,
    MessageHandler,
    CommandHandler,
)
import requests
import os
import re
import base64
import json
from io import BytesIO
from dotenv import load_dotenv
from utils.translations import trans

load_dotenv()


async def login(update: Update, context: ContextTypes.DEFAULT_TYPE):
    locale = update.message.from_user.language_code
    await update.message.reply_text(
        trans(locale, "Please enter your username and password separated by a space.")
    )


def main():
    app = ApplicationBuilder().token(os.getenv("TELEGRAM_TOKEN")).build()

    # Handler for replies to messages
    app.add_handler(CommandHandler("login", login))

    app.run_polling()


if __name__ == "__main__":
    main()
