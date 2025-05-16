import random
import string
from db.supabase import supabase


def generate_otp(length: int = 6, type: str = "alphanumeric") -> str:
    if type == "digits":
        return "".join(random.choices(string.digits, k=length))
    elif type == "letters":
        return "".join(random.choices(string.ascii_uppercase, k=length))
    elif type == "alphanumeric":
        return "".join(random.choices(string.ascii_uppercase + string.digits, k=length))
    else:
        raise ValueError("Invalid OTP type")


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
