/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>

#if 0 /// UNNEEDED by elogind
int make_salt(char **ret);

int hash_password_full(const char *password, void **cd_data, int *cd_size, char **ret);
static inline int hash_password(const char *password, char **ret) {
        return hash_password_full(password, NULL, NULL, ret);
}
#endif // 0
bool looks_like_hashed_password(const char *s);
#if 0 /// UNNEEDED by elogind
int test_password_one(const char *hashed_password, const char *password);
int test_password_many(char **hashed_password, const char *password);
#endif // 0
