from telegram import Update
from telegram.ext import ContextTypes
from telegram.constants import ParseMode
from db.supabase import supabase
from utils.translations import trans
from utils.utils import is_registered


def get_dashboard_info(telegram_id: int) -> dict:
    try:
        user_res = (
            supabase.table("persons")
            .select("id, username, is_premium")
            .eq("telegram_id", telegram_id)
            .execute()
        )

        if not user_res.data:
            return {}

        person = user_res.data[0]
        person_id = person["id"]

        wallet_res = (
            supabase.table("wallets")
            .select("balance")
            .eq("person_id", person_id)
            .execute()
        )

        sub_res = (
            supabase.table("subscriptions")
            .select("id")
            .eq("person_id", person_id)
            .execute()
        )

        return {
            "username": person["username"],
            "is_premium": person["is_premium"],
            "balance": wallet_res.data[0]["balance"] if wallet_res.data else 0,
            "subscriptions_count": len(sub_res.data),
        }

    except Exception as e:
        print(f"get_dashboard_info error: {e}")
        return {}


async def dashboard_logic(update: Update, context: ContextTypes.DEFAULT_TYPE):
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

    info = get_dashboard_info(user.id)

    if not info:
        await update.message.reply_text(
            trans(locale, "Error loading your dashboard information\\."),
            parse_mode=ParseMode.MARKDOWN_V2,
        )
        return

    await update.message.reply_text(
        trans(
            locale,
            "👤 *Username*: `[username]`\n💼 *Premium*: `[is_premium]`\n💰 *Balance*: `[balance] stars`\n📦 *Active Subscriptions*: `[subscriptions_count]`",
            {
                "username": info["username"],
                "is_premium": "Yes" if info["is_premium"] else "No",
                "balance": str(info["balance"]),
                "subscriptions_count": str(info["subscriptions_count"]),
            },
        ),
        parse_mode=ParseMode.MARKDOWN_V2,
    )
