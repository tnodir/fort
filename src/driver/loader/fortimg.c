/* Fort Firewall Driver Image Handling */

#include "fortimg.h"

#include <bcrypt.h>

#include "../fortutl.h"

#define FORT_IMAGE_VERIFY_DIGEST_SIZE 256

typedef struct fort_image_verify_arg
{
    const DWORD dataSize;
    const DWORD signatureSize;
    DWORD digestSize;

    const PUCHAR data;
    const PUCHAR signature;

    UCHAR digest[FORT_IMAGE_VERIFY_DIGEST_SIZE];
} FORT_IMAGE_VERIFY_ARG, *PFORT_IMAGE_VERIFY_ARG;

static const UCHAR g_publicKeyBlob[] = {
#include "fort.rsa.pub"
};

static NTSTATUS fort_image_verify_signature(PFORT_IMAGE_VERIFY_ARG iva)
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

            status = BCryptVerifySignature(keyHandle, &padInfo, iva->digest, iva->digestSize,
                    iva->signature, iva->signatureSize, BCRYPT_PAD_PKCS1);

            BCryptDestroyKey(keyHandle);
        }

        BCryptCloseAlgorithmProvider(algHandle, 0);
    }

    return status;
}

static NTSTATUS fort_image_hash_create(BCRYPT_ALG_HANDLE algHandle, PFORT_IMAGE_VERIFY_ARG iva)
{
    NTSTATUS status;

    DWORD hashDigestLen = 0;
    DWORD resultLen;
    status = BCryptGetProperty(algHandle, BCRYPT_HASH_LENGTH, (PUCHAR) &hashDigestLen,
            sizeof(hashDigestLen), &resultLen, 0);
    if (!NT_SUCCESS(status))
        return status;

    if (hashDigestLen > iva->digestSize)
        return STATUS_BUFFER_TOO_SMALL;

    BCRYPT_KEY_HANDLE hashHandle;
    status = BCryptCreateHash(algHandle, &hashHandle, NULL, 0, NULL, 0, 0);

    if (NT_SUCCESS(status)) {
        status = BCryptHashData(hashHandle, (PUCHAR) iva->data, iva->dataSize, 0);

        if (NT_SUCCESS(status)) {
            status = BCryptFinishHash(hashHandle, iva->digest, hashDigestLen, 0);

            if (NT_SUCCESS(status)) {
                iva->digestSize = hashDigestLen;
            }
        }

        BCryptDestroyHash(hashHandle);
    }

    return status;
}

static NTSTATUS fort_image_hash(PFORT_IMAGE_VERIFY_ARG iva)
{
    NTSTATUS status;

    BCRYPT_ALG_HANDLE algHandle;
    status = BCryptOpenAlgorithmProvider(&algHandle, BCRYPT_SHA256_ALGORITHM, NULL, 0);

    if (NT_SUCCESS(status)) {
        status = fort_image_hash_create(algHandle, iva);

        BCryptCloseAlgorithmProvider(algHandle, 0);
    }

    return status;
}

static NTSTATUS fort_image_verify(PFORT_IMAGE_VERIFY_ARG iva)
{
    NTSTATUS status;

    status = fort_image_hash(iva);

    if (NT_SUCCESS(status)) {
        status = fort_image_verify_signature(iva);
    }

    return status;
}

#define FORT_IMAGE_LOADER_SIZE_MIN    1024
#define FORT_IMAGE_PAYLOAD_INFO_SIZE  8
#define FORT_IMAGE_SIGNATURE_SIZE_MIN 512

FORT_API NTSTATUS fort_image_payload(
        const PUCHAR data, DWORD dataSize, PUCHAR *outPayload, DWORD *outPayloadSize)
{
    NTSTATUS status;

    /* The data: loader, payload, aligned signature and payload info */
    if (dataSize < FORT_IMAGE_LOADER_SIZE_MIN + FORT_IMAGE_PAYLOAD_INFO_SIZE)
        return STATUS_INVALID_IMAGE_FORMAT;

    const PUCHAR paylodInfo = data + dataSize - FORT_IMAGE_PAYLOAD_INFO_SIZE;
    const DWORD signatureSize = fort_le_u16_read(paylodInfo, 0);
    const DWORD alignedSignatureSize = fort_le_u16_read(paylodInfo, 2);
    const DWORD payloadSize = fort_le_u32_read(paylodInfo, 4);

#ifdef FORT_DEBUG
    LOG("Loader Image Load: size=%d signatureSize=%d alignedSignatureSize=%d payloadSize=%d\n",
            dataSize, signatureSize, alignedSignatureSize, payloadSize);
#endif

    if (signatureSize < FORT_IMAGE_SIGNATURE_SIZE_MIN || signatureSize > alignedSignatureSize)
        return STATUS_INVALID_IMAGE_FORMAT;

    const DWORD sizeMax = dataSize - FORT_IMAGE_LOADER_SIZE_MIN - FORT_IMAGE_PAYLOAD_INFO_SIZE;

    if (alignedSignatureSize > sizeMax || payloadSize > sizeMax - alignedSignatureSize)
        return STATUS_INVALID_IMAGE_FORMAT;

    const PUCHAR signature = paylodInfo - alignedSignatureSize;
    const PUCHAR payload = signature - payloadSize;

    FORT_IMAGE_VERIFY_ARG iva = {
        .dataSize = payloadSize,
        .signatureSize = signatureSize,
        .digestSize = sizeof(iva.digest),

        .data = payload,
        .signature = signature,
    };

    status = fort_image_verify(&iva);
    if (!NT_SUCCESS(status))
        return status;

    *outPayload = payload;
    *outPayloadSize = payloadSize;

    return STATUS_SUCCESS;
}
