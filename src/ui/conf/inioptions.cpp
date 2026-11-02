#include "inioptions.h"

IniOptions::IniOptions(Settings *settings) : MapSettings(settings) { }

void IniOptions::saveInitialAutoLearnSeconds(int v)
{
    if (initialAutoLearnSeconds() == v)
        return;

    setInitialAutoLearnSeconds(v);
    saveAndClear();
}
