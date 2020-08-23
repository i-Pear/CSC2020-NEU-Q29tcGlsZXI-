#include "RegControl.h"

RegController regCtrl;

RegRecord::RegRecord(RegType reg, int id, int addr) : reg(reg), id(id), addr(addr), update(false){}

void RegRecord::load(bool silent) const{
    if(addr>50){
        if(silent)push_r12();
        writer<<"ldr r12,="<<addr<<endl;
        writer<<"ldr "<<reg2str(reg)<<",[r10,r12]"<<endl;
        if(silent)pop_r12();
    } else{
        writer<<"ldr "<<reg2str(reg)<<",[r9,#"<<addr<<"]"<<endl;
    }
}

void RegRecord::save(bool silent) const{
    if(!update)return;
    if(addr>50){
        if(silent)push_r12();
        writer<<"ldr r12,="<<addr<<endl;
        writer<<"str "<<reg2str(reg)<<",[r10,r12]"<<endl;
        if(silent)pop_r12();
    } else{
        writer<<"str "<<reg2str(reg)<<",[r9,#"<<addr<<"]"<<endl;
    }
}

RegType RegController::load(int id, int addr, bool realLoad){
    // if has already loaded
    for(auto i=occupied.begin(); i!=occupied.end(); i++){
        if(i->id==id){
            auto rec=*i;
            occupied.erase(i);
            occupied.push_front(rec);
            cout<<"var "<<tokenizer[id]<<" has already in register "<<reg2str(rec.reg)<<endl;
            return rec.reg;
        }
    }
    // load var r2~r8: 7 ones
    if(occupied.size()<7){
        // has empty register -> find an empty one
        bool exists[9]={};
        for(auto&i:occupied){
            exists[i.reg]=true;
        }
        for(int i=2; i<=8; i++){
            if(!exists[i]){
                RegRecord rec={(RegType) i, id, addr};
                if(realLoad)rec.load();
                occupied.push_front(rec);
                cout<<"var "<<tokenizer[id]<<" detected empty register "<<reg2str(rec.reg)<<endl;
                return (RegType) i;
            }
        }
    } else{
        // free one
        auto old=occupied.back();
        occupied.pop_back();
        old.save();
        RegRecord rec={old.reg, id, addr};
        if(realLoad)rec.load();
        occupied.push_front(rec);
        cout<<"var "<<tokenizer[id]<<" replaced with register "<<reg2str(rec.reg)<<endl;
        return (RegType) rec.reg;
    }
}

RegType RegController::load_silent(int id, int addr, bool realLoad){
    // if has already loaded
    for(auto i=occupied.begin(); i!=occupied.end(); i++){
        if(i->id==id){
            auto rec=*i;
            occupied.erase(i);
            occupied.push_front(rec);
            cout<<"var "<<tokenizer[id]<<" has already in register "<<reg2str(rec.reg)<<endl;
            return rec.reg;
        }
    }
    // load var r2~r8: 7 ones
    if(occupied.size()<7){
        // has empty register -> find an empty one
        bool exists[9]={};
        for(auto&i:occupied){
            exists[i.reg]=true;
        }
        for(int i=2; i<=8; i++){
            if(!exists[i]){
                RegRecord rec={(RegType) i, id, addr};
                if(realLoad)rec.load(true);
                occupied.push_front(rec);
                cout<<"var "<<tokenizer[id]<<" detected empty register "<<reg2str(rec.reg)<<endl;
                return (RegType) i;
            }
        }
    } else{
        // free one
        auto old=occupied.back();
        occupied.pop_back();
        old.save(true);
        RegRecord rec={old.reg, id, addr};
        if(realLoad)rec.load(true);
        occupied.push_front(rec);
        cout<<"var "<<tokenizer[id]<<" replaced with register "<<reg2str(rec.reg)<<endl;
        return (RegType) rec.reg;
    }
}

void RegController::clear(bool skipSavingLocal){
    for(auto&i:occupied){
        if(!skipSavingLocal){
            i.save();
        } else{
            if(i.addr>50)i.save();
        }
    }
    cout<<"#### registers cleared. ####"<<endl;
    occupied.clear();
}

void RegController::fake_clear(){
    for(auto&i:occupied){
        i.save();
    }
    cout<<"#### registers fakely cleared. ####"<<endl;
}

void RegController::mark_update(int id){
    for(auto&i:occupied){
        if(i.id==id){
            i.update=true;
            return;
        }
    }
    throw CompileERROR("Requesting to mark update status: id not loaded.");
}