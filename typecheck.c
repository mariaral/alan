#include <stdlib.h>

#include "quad.h"
#include "error.h"

bool int_or_byte(Type a,Type b)
{
    return (((a==typeInteger)||(a==typeChar))&&((b==typeInteger)||(b==typeChar)));
}

bool equalArrays(Type a,Type b)
{
    if(a->kind!=TYPE_ARRAY) return false;
    else if (b->kind==TYPE_IARRAY) return true;
    else return false;
}

bool unknownType(Type a, Type b) {
    return ((a==typeUnknown)||(b==typeUnknown));
}



Type paramType(SymbolEntry* arg)
{
    return (arg->u.eParameter.type);
}

bool paramChecked(bool* many, SymbolEntry** point_arg, varstr exp)
{
    SymbolEntry* entry;
    SymbolEntry* arg; 
    bool ret;

    arg = *point_arg;

    if(*many==true) return false;
    if(arg==NULL) {
        error("Too many arguments at call");
        *many = true;
        return false;
    }
    if(arg->u.eParameter.mode==PASS_BY_REFERENCE) {
        if(exp.place.placeType==ENTRY) {
            entry = exp.place.entry;
            if((entry->entryType!=ENTRY_VARIABLE)&&(entry->entryType!=ENTRY_PARAMETER)) {
                error("Cannot pass that type of expression by reference");
                ret = false;
                goto out;
            }
        }
    }
    if((!equalType(exp.type,paramType(arg)))&&(!equalArrays(exp.type,paramType(arg)))) {
        error("Wrong type of parameter");
        ret = false;
        goto out;
    }
    else {
        ret = true;
        goto out;
    }
out:
    *point_arg = arg->u.eParameter.next;
    return ret;
}

bool paramString(bool* many, SymbolEntry** point_arg)
{
    SymbolEntry* arg;
    bool ret;

    arg = *point_arg;

    if(*many==true) return false;
    if(arg==NULL) {
        error("Too many arguments at call");
        *many = true;
        return false;
    }
    if(!equalType(paramType(arg),typeIArray(typeChar))) {
        error("Wrong type of parameter");
        ret = false;
        goto out;
    }
    else {
        ret = true;
        goto out;
    }
out:
    *point_arg = arg->u.eParameter.next;
    return ret;
}
