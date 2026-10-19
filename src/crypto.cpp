// Copyright (C) 2026 Le Tuan Kiet (aujiz11)
// OpenSSL adapter for TOTP, originally authored by Games.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "crypto.h"

#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/params.h>
#include <openssl/provider.h>
#include <openssl/rand.h>
#include <memory>

namespace crypto {
    bool constant_time_equal(const void* left, const void* right, std::size_t length) noexcept
    {
        return left && right && CRYPTO_memcmp(left, right, length) == 0;
    }

    bool random_secret(SecretBytes& output)
    {
        secure_erase(output.data(), output.size());
        ScopedCleanse failed_output(output.data(), output.size());
        if (OpenSSL_version_num() != OPENSSL_VERSION_NUMBER) return false;
        
        std::unique_ptr<OSSL_LIB_CTX, decltype(&OSSL_LIB_CTX_free)> library(OSSL_LIB_CTX_new(), OSSL_LIB_CTX_free);
        if (!library) return false;
        
        std::unique_ptr<OSSL_PROVIDER, decltype(&OSSL_PROVIDER_unload)> provider(OSSL_PROVIDER_load(library.get(), "default"), OSSL_PROVIDER_unload);
        if (!provider || EVP_set_default_properties(library.get(), "provider=default") != 1 ||
            RAND_priv_bytes_ex(library.get(), output.data(), output.size(), 256) != 1) return false;
        
        failed_output.release();
        return true;
    }

    void secure_erase(void* memory, std::size_t length) noexcept
    {
        if (memory && length != 0) OPENSSL_cleanse(memory, length);
    }

    ScopedCleanse::~ScopedCleanse() noexcept
    {
        secure_erase(memory_, length_);
    }

    bool hmac_sha1(const std::uint8_t* key, std::size_t key_length,
                const std::uint8_t* data, std::size_t data_length,
                Sha1Digest& output)
    {
        secure_erase(output.data(), output.size());
        ScopedCleanse failed_output(output.data(), output.size());
        if (!key || key_length == 0 || (!data && data_length != 0)) return false;
        if (OpenSSL_version_num() != OPENSSL_VERSION_NUMBER) return false;

        std::unique_ptr<OSSL_LIB_CTX, decltype(&OSSL_LIB_CTX_free)>library(OSSL_LIB_CTX_new(), OSSL_LIB_CTX_free);
        if (!library) return false;

        std::unique_ptr<OSSL_PROVIDER, decltype(&OSSL_PROVIDER_unload)>provider(OSSL_PROVIDER_load(library.get(), "default"), OSSL_PROVIDER_unload);
        if (!provider) return false;
        
        std::unique_ptr<EVP_MAC, decltype(&EVP_MAC_free)>mac(EVP_MAC_fetch(library.get(), "HMAC", "provider=default"), EVP_MAC_free);
        if (!mac) return false;
        
        std::unique_ptr<EVP_MAC_CTX, decltype(&EVP_MAC_CTX_free)>context(EVP_MAC_CTX_new(mac.get()), EVP_MAC_CTX_free);
        if (!context) return false;
        
        char digest[] = "SHA1";
        char properties[] = "provider=default";
        OSSL_PARAM parameters[] = {
            OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest, 0),
            OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_PROPERTIES, properties, 0),
            OSSL_PARAM_construct_end()
        };
        
        std::size_t length = 0;
        if (EVP_MAC_init(context.get(), key, key_length, parameters) != 1 ||
            (data_length != 0 && EVP_MAC_update(context.get(), data, data_length) != 1) ||
            EVP_MAC_final(context.get(), output.data(), &length, output.size()) != 1 ||
            length != output.size()) {
            return false;
        }
        failed_output.release();
        return true;
    }
}
