#include "zone.h"

bool Zone::isNameEqual(const Zone &o) const
{
    return zoneName == o.zoneName;
}

bool Zone::isOptionsEqual(const Zone &o) const
{
    return enabled == o.enabled && isSourceEqual(o);
}

bool Zone::isSourceEqual(const Zone &o) const
{
    return customUrl == o.customUrl && sourceCode == o.sourceCode && url == o.url
            && formData == o.formData && textInline == o.textInline;
}
