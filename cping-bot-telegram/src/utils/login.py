from telegram import Update, User
from telegram.ext import ContextTypes
from telegram.constants import ParseMode
from db.supabase import supabase
from utils.translations import trans


def is_registered(telegram_id: int) -> bool:
    try:
        response = (
            supabase.table("persons")
            .select("*")
            .eq("telegram_id", telegram_id)
            .execute()
        )
        return len(response.data) > 0
    except Exception as e:
        print(f"is_registered error: {e}")
        return False


def insert_person(
    telegram_id: int,
    first_name: str,
    last_name: str,
    username: str,
    language_code: str,
    is_premium: bool,
) -> int:
    try:
        person_data: dict[str, any] = {
            "telegram_id": telegram_id,
            "first_name": first_name,
            "last_name": last_name,
            "username": username,
            "language_code": language_code,
            "is_premium": is_premium,
        }

        response = supabase.table("persons").insert([person_data]).execute()
        return response.data[0]["id"]
    except Exception as e:
        print(f"insert_person error: {e}")
        return -1


def insert_otp(person_id: int, otp: str) -> int:
    try:
        otp_data: dict[str, any] = {"person_id": person_id, "otp": otp}
        response = supabase.table("otps").insert([otp_data]).execute()
        return response.data[0]["id"]
    except Exception as e:
        print(f"insert_otp error: {e}")
        return -1


def get_person_id(telegram_id: int) -> int:
    try:
        response = (
            supabase.table("persons")
            .select("*")
            .eq("telegram_id", telegram_id)
            .execute()
        )
        return response.data[0]["id"]
    except Exception as e:
        print(f"get_person_id error: {e}")
        return -1


def update_if_changed(person_id: int, new_data: dict[str, any]) -> bool:
    try:
        response = (
            supabase.table("persons")
            .select(
                "first_name, last_name, username, language_code, is_premium, telegram_id"
            )
            .eq("id", person_id)
            .limit(1)
            .execute()
        )

        if not response.data:
            return

        current = response.data[0]
        updates = {}
        has_change = False

        for field in [
            "first_name",
            "last_name",
            "username",
            "language_code",
            "is_premium",
        ]:
            if str(current[field]) != str(new_data[field]):
                updates[field] = new_data[field]
                has_change = True

        if has_change:
            supabase.table("persons").update(updates).eq("id", person_id).execute()
            log_data = {
                "person_id": person_id,
                "telegram_id": current["telegram_id"],
                "first_name": current["first_name"],
                "last_name": current["last_name"],
                "username": current["username"],
                "language_code": current["language_code"],
                "is_premium": current["is_premium"],
            }
            supabase.table("person_logs").insert([log_data]).execute()

    except Exception as e:
        print(f"update_if_changed error: {e}")


async def login_logic(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    try:
        user: User = update.message.from_user
        locale: str = user.language_code
        person_id: int = -1
        otp: str = ""

        if not is_registered(user.id):
            await update.message.reply_text(
                trans(locale, "You are not registered\\!"),
                parse_mode=ParseMode.MARKDOWN_V2,
            )
            person_id = insert_person(
                user.id,
                user.first_name or "-",
                user.last_name or "-",
                user.username or "-",
                user.language_code or "-",
                user.is_premium or False,
            )

            if person_id == -1:
                await update.message.reply_text(
                    trans(locale, "Error inserting person or already registered\\!"),
                    parse_mode=ParseMode.MARKDOWN_V2,
                )
                return

        try:
            person_id = get_person_id(user.id)
            if person_id == -1:
                await update.message.reply_text(
                    trans(locale, "Error logging in\\!"),
                    parse_mode=ParseMode.MARKDOWN_V2,
                )
                return

            update_if_changed(
                person_id,
                {
                    "first_name": user.first_name or "-",
                    "last_name": user.last_name or "-",
                    "username": user.username or "-",
                    "language_code": user.language_code or "-",
                    "is_premium": user.is_premium or False,
                },
            )

            try:
                response = supabase.rpc(
                    "fun_generate_unique_otp", {"person_id_input": person_id}
                ).execute()
                otp = response.data
            except Exception as e:
                if e.message == "You must wait before generating another OTP.":
                    await update.message.reply_text(
                        trans(
                            locale,
                            "Please wait 5 minute before generating a new OTP\\.",
                        ),
                        parse_mode=ParseMode.MARKDOWN_V2,
                    )
                    return
                print(f"generate_unique_otp error: {e}")
                print(f"generate_unique_otp response: {response}")
                return -1

            await update.message.reply_text(
                trans(
                    locale,
                    "Your OTP is: `[otp]`\nPlease enter this OTP in application and click on Login button\\.",
                    {"otp": otp},
                ),
                parse_mode=ParseMode.MARKDOWN_V2,
            )
        except Exception as e:
            print(f"login_logic send OTP error: {e}")
            await update.message.reply_text(
                trans(locale, "Error sending OTP\\!"), parse_mode=ParseMode.MARKDOWN_V2
            )
            return

    except Exception as e:
        print(f"login_logic main error: {e}")
        await update.message.reply_text(
            trans(locale, "Error logging in\\!"), parse_mode=ParseMode.MARKDOWN_V2
        )
        return
