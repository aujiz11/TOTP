# OpenSSL 3.5.9 overlay provenance

This recipe is copied from `microsoft/vcpkg` at
`44ccfd88b3eca6929d6d16398c7b19945c29a5f6` (MIT license;
see openssl/LICENSE-vcpkg.txt). Upstream default port is 3.6.5#1.

Project changes: port version is 3.5.9; source acquisition uses the official
release archive rather than GitHub's generated snapshot, with pinned SHA512.
The upstream platform recipes, patches and helper dependencies are retained.

Archive URL:
https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz

SHA256 verified against the official release checksum file:
603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a

SHA512 enforced by vcpkg:
c4c8136fd2c98d13bb85a57bfb0def25c906f47f1fe97675cd114594944eab957b5b81a3e0f48826e1a0f093afff5e83766b539d49768f6db68310fd678d7de6

OpenSSL source is Apache-2.0; the port installs its copyright notice.
This overlay exists because the pinned built-in registry does not contain
3.5.9. Update version, checksums and qualification evidence together; do not
silently move the project to 3.6 or a floating port release.
