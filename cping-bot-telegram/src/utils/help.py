from telegram import Update
from telegram.ext import ContextTypes
from telegram.constants import ParseMode
from utils.translations import trans
from utils.utils import is_registered


async def help_logic(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user = update.message.from_user
    locale = user.language_code

    if not is_registered(user.id):
        await update.message.reply_text(
            trans(
                locale,
                "You must be registered to use this service\\.\nPlease use /login to register your account\\.",
            ),
            parse_mode=ParseMode.MARKDOWN_V2,
        )
        return

    await update.message.reply_text(
        trans(
            locale,
            "Need help or found an issue\\?\n\nIf you have any questions, suggestions, or bug reports, feel free to contact us at: @cping\\_o",
        ),
        parse_mode=ParseMode.MARKDOWN_V2,
    )
