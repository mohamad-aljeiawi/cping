from supabase import create_client, Client
from utils.config import config


supabase: Client = create_client(config["supabase_url"], config["supabase_key"])
