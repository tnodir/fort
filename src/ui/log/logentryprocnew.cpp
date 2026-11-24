#include "logentryprocnew.h"

#include <util/fileutil.h>

QString LogEntryProcNew::inheritPath() const
{
    return FileUtil::kernelPathToPath(m_inheritKernelPath);
}
