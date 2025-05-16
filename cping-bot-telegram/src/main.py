from telegram import Update
from telegram.ext import (
    ApplicationBuilder,
    ContextTypes,
    CommandHandler,
)
from utils.config import config
from utils.login import login_logic
from utils.start import start_logic
from utils.help import help_logic
from utils.dashboard import dashboard_logic


async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await start_logic(update, context)


async def login(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await login_logic(update, context)


async def help(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await help_logic(update, context)


async def dashboard(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await dashboard_logic(update, context)


def main():
    app = ApplicationBuilder().token(config["telegram_token"]).build()
    app.add_handler(CommandHandler("start", start))
    app.add_handler(CommandHandler("login", login))
    app.add_handler(CommandHandler("help", help_logic))
    app.add_handler(CommandHandler("dashboard", dashboard_logic))
    app.run_polling()


if __name__ == "__main__":
    main()
