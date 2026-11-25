#ifndef INISETTER_H
#define INISETTER_H

#include <functional>

// The setter of the changed option of the ini got by the function: marks the ini edited
template<typename Ini, typename T, typename IniFunc, typename EditedFunc>
std::function<void(T)> iniSetter(const IniFunc &iniFunc, T (Ini::*iniValue)() const,
        void (Ini::*setIniValue)(T), const EditedFunc &setEdited)
{
    return [=](T v) {
        Ini &ini = iniFunc();
        if ((ini.*iniValue)() == v)
            return;

        (ini.*setIniValue)(v);
        setEdited();
    };
}

#endif // INISETTER_H
