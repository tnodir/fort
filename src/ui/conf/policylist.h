#ifndef POLICYLIST_H
#define POLICYLIST_H

enum PolicyListType {
    PolicyListInvalid = -1,
    PolicyListNone = 0,
    PolicyListPresetLibrary,
    PolicyListPresetApp,
    PolicyListGlobalBeforeApp,
    PolicyListGlobalAfterApp,
    PolicyListCount
};

#endif // POLICYLIST_H
