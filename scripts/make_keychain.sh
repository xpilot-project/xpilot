#!/bin/bash

# Largely based on the scripts in this:
# https://github.com/marketplace/actions/import-code-signing-certificates
# 
# Assumes the dev key and dev key password are stored in variables as
# defined below, and the key is a base64 environment variable:
# cat ~/dev_signing_certificate.p12 | openssl base64 -A| pbcopy
set -e -u -x

# CERTIFICATE=`cat ~/dev_signing_certificate.p12 | openssl base64 -A`
# CERTIFICATE_PASSWORD="FIXME"

KEYCHAIN_NAME="Signing keychain"
KEYCHAIN_PASSWORD=`openssl rand -base64 12`

# delete keychain
# security delete-keychain "${KEYCHAIN_NAME}"

# create keychain
security create-keychain -p "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_NAME}"
security set-keychain-settings -lut 21600 "${KEYCHAIN_NAME}"

# unlock keychain
security unlock-keychain -p "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_NAME}"

# import certificate
CERT_ON_DISK=/tmp/cert.p12
echo ${CERTIFICATE} | openssl base64 -A -d >"${CERT_ON_DISK}"
security import "${CERT_ON_DISK}" -k "${KEYCHAIN_NAME}" -f pkcs12 -A -T /usr/bin/codesign -T /usr/bin/security -P "${CERTIFICATE_PASSWORD}"

# set partition list
security set-key-partition-list -S apple-tool:,apple: -k "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_NAME}"

# update keychain list
security list-keychains -d user -s "${KEYCHAIN_NAME}" login.keychain