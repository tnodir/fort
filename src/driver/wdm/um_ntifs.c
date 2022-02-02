#include "um_ntifs.h"

NTSTATUS RtlDowncaseUnicodeString(PUNICODE_STRING destinationString, PCUNICODE_STRING sourceString,
        BOOLEAN allocateDestinationString)
{
    UNUSED(destinationString);
    UNUSED(sourceString);
    UNUSED(allocateDestinationString);
    return STATUS_SUCCESS;
}
