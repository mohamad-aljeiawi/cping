from typing import Optional


def trans(locale: str, key: str, vars: Optional[dict[str, str]] = None) -> str:
    if not key:
        raise ValueError("No key provided to translate()")

    vars = vars or {}

    if "ar" in locale:
        text = translations.get(key, key)
    else:
        text = key

    for k, v in vars.items():
        text = text.replace(f"[{k}]", v)

    return text


translations: dict[str, str] = {
    # login
    "You are not registered\\!": "أنت غير مسجل\\!",
    "Error inserting person or already registered\\!": "حدث خطأ أثناء إدراج المستخدم أو قد تم التسجيل بالفعل\\!",
    "Error inserting OTP or already registered\\!": "حدث خطأ أثناء إدراج رمز التحقق أو قد تم التسجيل بالفعل\\!",
    "Error sending OTP\\!": "حدث خطأ أثناء إرسال رمز التحقق\\!",
    "Error logging in\\!": "حدث خطأ أثناء تسجيل الدخول\\!",
    "Your OTP is: [otp]`\nPlease enter this OTP in application and click on Login button\\.": "رمز التحقق الخاص بك هو: [otp]\nالرجاء إدخال هذا الرمز في التطبيق والضغط على زر تسجيل الدخول\\.",
    "Please wait 5 minute before generating a new OTP\\.": "الرجاء الإنتظار 5 دقائق قبل إنشاء رمز تحقق جديد\\.",
    # start
    "You must be registered to use this service.\nPlease use /login to register your account.": "يجب عليك تسجيل الدخول لاستخدام هذه الخدمة\\.\nالرجاء استخدام /login لتسجيل حسابك\\.",
    "You must set a Telegram username before using this service.\nPlease go to your Telegram settings, create a username, and try again.": "يجب عليك تعيين اسم المستخدم للتواصل قبل استخدام هذه الخدمة\\.\nالرجاء الذهاب إلى إعدادات Telegram الخاصة بك وإنشاء اسم مستخدم وإعادة المحاولة\\.",
    "Welcome to our service!\nTo get started, please use /login to register your account.": "مرحبا بك في خدمتنا\\!\nللبدء، يرجى استخدام /login لتسجيل حسابك\\.",
    "Welcome back!\nYou can use the following commands:\n/dashboard - View your account information\n/login - Login to your account generate OTP\n/help - Get support and assistance": "مرحبا بك مرة أخرى\\!\nيمكنك استخدام الأوامر التالية:\n/dashboard \\- عرض معلومات حسابك\n/login \\- تسجيل الدخول لتوليد رمز التحقق\n/help \\- الحصول على الدعم والمساعدة",
    # main
    # help
    "Need help or found an issue?\n\nIf you have any questions, suggestions, or bug reports, feel free to contact us at: @cping_o": "تحتاج المساعدة أو وجود مشكلة\\?\n\nإذا لديك أي أسئلة، اقتراحات، أو تقارير عن أعطال، يرجى الاتصال بنا على: @cping\\_o",
    # dashboard
    "Error loading your dashboard information.": "حدث خطأ أثناء تحميل معلومات لوحة التحكم الخاصة بك\\.",
    "👤 *Username*: `[username]`\n💼 *Premium*: `[is_premium]`\n💰 *Balance*: `[balance] stars`\n📦 *Active Subscriptions*: `[subscriptions_count]`": "👤 *الاسم المستخدم*: `[username]`\n💼 *البرنامج المدفوع*: `[is_premium]`\n💰 *الرصيد*: `[balance] stars`\n📦 *الاشتراكات النشطة*: `[subscriptions_count]`",
}
