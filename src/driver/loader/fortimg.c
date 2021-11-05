/* Fort Firewall Driver Image Handling */

#include "fortimg.h"

#include <bcrypt.h>

#include "../fortutl.h"

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

FORT_API NTSTATUS fort_image_load(const PUCHAR data, DWORD dataSize, PUCHAR *image, DWORD *outSize)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Image Load: %d\n", dataSize);

    const PUCHAR paylodInfo = data + dataSize - 8;
    const int signatureSize = fort_le_u16_read(paylodInfo, 0);
    const int alignedSignatureSize = fort_le_u16_read(paylodInfo, 2);
    const int payloadSize = fort_le_u32_read(paylodInfo, 4);

    const PUCHAR signature = paylodInfo - alignedSignatureSize;
    const PUCHAR payload = signature - payloadSize;

    if (signatureSize < 512 || payload - data < 1024)
        return STATUS_INVALID_IMAGE_FORMAT;

    UCHAR digest[1024];
    DWORD digestSize = sizeof(digest);
    status = fort_image_hash(signature, signatureSize, digest, &digestSize);
    if (!NT_SUCCESS(status))
        return status;

    return STATUS_SUCCESS;
}
