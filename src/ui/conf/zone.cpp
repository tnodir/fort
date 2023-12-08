#include "zone.h"

bool Zone::isNameEqual(const Zone &o) const
{
    return zoneName == o.zoneName;
}

bool Zone::isOptionsEqual(const Zone &o) const
{
    return enabled == o.enabled && customUrl == o.customUrl && sourceCode == o.sourceCode
            && url == o.url && formData == o.formData && textInline == o.textInline;
}
