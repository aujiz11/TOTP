#if defined totp_included
    #endinput
#endif
#define totp_included

// It is not recommended to change the values ​​below.
#define TOTP_CODE_LENGTH         (6)
#define TOTP_CODE_SIZE           (TOTP_CODE_LENGTH + 1)
#define TOTP_SECRET_LENGTH       (32)
#define TOTP_SECRET_SIZE         (TOTP_SECRET_LENGTH + 1)
#define TOTP_VERSION_SIZE        (32)
#define TOTP_MAX_SECRET_LENGTH   (160)

#define TOTP_DEFAULT_SKEW        (1)
#define TOTP_MAX_SKEW            (2)

enum {
    TOTP_ERROR = -1,
    TOTP_MISMATCH,
    TOTP_SUCCESS
};

native GoogleAuthenticatorCode(const secret[], timestamp);
native TOTP_GenerateCode(const secret[], code[], code_size = TOTP_CODE_SIZE);
native TOTP_VerifyCode(const secret[], const code[], skew = TOTP_DEFAULT_SKEW);
native TOTP_GenerateSecret(secret[], secret_size = TOTP_SECRET_SIZE);
native TOTP_GetVersion(version[], version_size = TOTP_VERSION_SIZE);