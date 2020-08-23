#pragma once

#include "Utils.h"
#include "Tokenizer.h"
#include "AsmHelper.h"

class RegRecord{
public:
    RegRecord(RegType reg, int id, int addr);

    RegType reg;
    int id;
    int addr;
    bool update;

    void load(bool silent=false) const;

    void save(bool silent=false) const;
};

class RegController{
public:
    /**
     * Controls variable <-> register
     * R0,R1,R11,R12 is for temp usages
     * variables store in r2~r8 : 7 ones
     **/
    list<RegRecord> occupied;

    RegType load(int id, int addr, bool realLoad=true);

    RegType load_silent(int id, int addr, bool realLoad=true);

    void clear(bool skipSavingLocal=false);

    void mark_update(int id);

    void fake_clear();
};

extern RegController regCtrl;