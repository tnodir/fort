/* Fort Firewall Driver Image Handling */

#include "fortimg.h"

#include <bcrypt.h>

#include "../fortutl.h"

static const UCHAR g_publicKeyBlob[] = {
#include "fort.rsa.pub"
};

static NTSTATUS fort_image_verify_signature(
        const PUCHAR signature, DWORD signatureSize, const PUCHAR digest, DWORD digestSize)
{
    NTSTATUS status;

    BCRYPT_ALG_HANDLE algHandle;
    status = BCryptOpenAlgorithmProvider(&algHandle, BCRYPT_RSA_ALGORITHM, NULL, 0);

    if (NT_SUCCESS(status)) {
        BCRYPT_KEY_HANDLE keyHandle;
        status = BCryptImportKeyPair(algHandle, NULL, BCRYPT_RSAPUBLIC_BLOB, &keyHandle,
                (PUCHAR) g_publicKeyBlob, sizeof(g_publicKeyBlob), 0);

        if (NT_SUCCESS(status)) {
            BCRYPT_PKCS1_PADDING_INFO padInfo;
            padInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

            status = BCryptVerifySignature(keyHandle, &padInfo, digest, digestSize, signature,
                    signatureSize, BCRYPT_PAD_PKCS1);

            BCryptDestroyKey(keyHandle);
        }

        BCryptCloseAlgorithmProvider(algHandle, 0);
    }

    return status;
}

static NTSTATUS fort_image_hash_create(BCRYPT_ALG_HANDLE algHandle, const PUCHAR data,
        DWORD dataSize, PUCHAR digest, DWORD *digestSize)
{
    NTSTATUS status;

    DWORD hashDigestLen = 0;
    DWORD resultLen;
    status = BCryptGetProperty(algHandle, BCRYPT_HASH_LENGTH, (PUCHAR) &hashDigestLen,
            sizeof(hashDigestLen), &resultLen, 0);
    if (!NT_SUCCESS(status))
        return status;

    if (hashDigestLen > *digestSize)
        return STATUS_BUFFER_TOO_SMALL;

    BCRYPT_KEY_HANDLE hashHandle;
    status = BCryptCreateHash(algHandle, &hashHandle, NULL, 0, NULL, 0, 0);

    if (NT_SUCCESS(status)) {
        status = BCryptHashData(hashHandle, (PUCHAR) data, dataSize, 0);

        if (NT_SUCCESS(status)) {
            status = BCryptFinishHash(hashHandle, digest, hashDigestLen, 0);

            if (NT_SUCCESS(status)) {
                *digestSize = hashDigestLen;
            }
        }

        BCryptDestroyHash(hashHandle);
    }

    return status;
}

static NTSTATUS fort_image_hash(const PUCHAR data, DWORD dataSize, PUCHAR digest, DWORD *digestSize)
{
    NTSTATUS status;

    BCRYPT_ALG_HANDLE algHandle;
    status = BCryptOpenAlgorithmProvider(&algHandle, BCRYPT_SHA256_ALGORITHM, NULL, 0);

    if (NT_SUCCESS(status)) {
        status = fort_image_hash_create(algHandle, data, dataSize, digest, digestSize);

        BCryptCloseAlgorithmProvider(algHandle, 0);
    }

    return status;
}

static NTSTATUS fort_image_verify(
        const PUCHAR data, DWORD dataSize, const PUCHAR signature, DWORD signatureSize)
{
    NTSTATUS status;

    UCHAR digest[256];
    DWORD digestSize = sizeof(digest);
    status = fort_image_hash(data, dataSize, digest, &digestSize);

    if (NT_SUCCESS(status)) {
        status = fort_image_verify_signature(signature, signatureSize, digest, digestSize);
    }

    return status;
}

FORT_API NTSTATUS fort_image_payload(
        const PUCHAR data, DWORD dataSize, PUCHAR *outPayload, DWORD *outPayloadSize)
{
    NTSTATUS status;

    const PUCHAR paylodInfo = data + dataSize - 8;
    const int signatureSize = fort_le_u16_read(paylodInfo, 0);
    const int alignedSignatureSize = fort_le_u16_read(paylodInfo, 2);
    const int payloadSize = fort_le_u32_read(paylodInfo, 4);

#ifdef FORT_DEBUG
    LOG("Loader Image Load: size=%d signatureSize=%d alignedSignatureSize=%d payloadSize=%d\n",
            dataSize, signatureSize, alignedSignatureSize, payloadSize);
#endif

    const PUCHAR signature = paylodInfo - alignedSignatureSize;
    const PUCHAR payload = signature - payloadSize;

    if (signatureSize < 512 || payload - data < 1024)
        return STATUS_INVALID_IMAGE_FORMAT;

    status = fort_image_verify(payload, payloadSize, signature, signatureSize);
    if (!NT_SUCCESS(status))
        return status;

    *outPayload = payload;
    *outPayloadSize = payloadSize;

    return STATUS_SUCCESS;
}
