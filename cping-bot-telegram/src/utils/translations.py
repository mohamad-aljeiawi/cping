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
        text = text.replace(f"::{k}::", v)

    return text


translations: dict[str, str] = {
    "hello hi?": "مرحبا بك في البوت",
    "Please enter your username and password separated by a space.": "الرجاء إدخال اسم المستخدم وكلمة المرور مفصولة بمسافة.",
}
