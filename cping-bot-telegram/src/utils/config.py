import os
from dotenv import load_dotenv

load_dotenv()

config = {
    "supabase_url": os.environ.get("SUPABASE_URL"),
    "supabase_key": os.environ.get("SUPABASE_KEY"),
    "telegram_token": os.environ.get("TELEGRAM_TOKEN"),
}
