#pragma once
constexpr unsigned char kBrandingEncrypted[] = {
    0x0a, 0x0b, 0x62, 0x0b, 0x0c, 0x62, 0x01, 0x12,
    0x0b, 0x0c, 0x05, 0x62, 0x3e, 0x62, 0x02, 0x21,
    0x32, 0x2b, 0x2c, 0x25, 0x1d, 0x21, 0x2a, 0x23,
    0x2c, 0x2c, 0x27, 0x2e, 0x00};

constexpr unsigned char kXorKey = 0x42;

inline const char *get_protected_branding()
{
    static char decrypted[32] = {0};
    static bool initialized = false;

    if (!initialized)
    {
        for (int i = 0; i < sizeof(kBrandingEncrypted) - 1; ++i)
        {
            decrypted[i] = kBrandingEncrypted[i] ^ kXorKey;
        }
        decrypted[sizeof(kBrandingEncrypted) - 1] = '\0';
        initialized = true;
    }

    return decrypted;
}

inline bool verify_branding_integrity()
{
    const char *branding = get_protected_branding();
    int checksum = 0;
    for (int i = 0; branding[i] != '\0'; ++i)
    {
        checksum += (unsigned char)branding[i];
    }
    return checksum == 2334;
}