/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flmeth.cpp
    \brief Method processing of flext base class.
*/
 
#ifndef __FLEXT_METH_CPP
#define __FLEXT_METH_CPP

#include "flext.h"
#include "flinternal.h"
#include <cstring>
#include <cstdarg>

#include "flpushns.h"

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::MethItem::MethItem(AttrItem *conn):
    Item(conn),index(0),
    argc(0),args(NULL)
    ,fun(NULL)
{}

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::MethItem::~MethItem()
{ 
    if(args) delete[] args; 
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::MethItem::SetArgs(methfun _fun,int _argc,int *_args)
{
    fun = _fun;
    if(args) delete[] args;
    argc = _argc,args = _args;
}

/*! \brief Add a method to the queue
*/
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::AddMethod(ItemCont *ma,int inlet,const t_symbol *tag,methfun fun,int tp,...)
{
#ifdef FLEXT_LOG_MSGS
	post("addmethod %i:%s",inlet,GetString(tag));
#endif

    va_list marker; 

    // at first just count the arg type list (in argc)
    int argc = 0;
    va_start(marker,tp);
    int *args = NULL,arg = tp;
    for(; arg != a_null; ++argc) arg = va_arg(marker,int);
    va_end(marker);
    
    if(argc > 0) {
        if(argc > FLEXT_MAXMETHARGS) {
            error("flext - method %s: only %i arguments are type-checkable: use variable argument list for more",tag?GetString(tag):"?",FLEXT_MAXMETHARGS);
            argc = FLEXT_MAXMETHARGS;
        }

        args = new int[argc];

        va_start(marker,tp);
        int a = tp;
        for(int ix = 0; ix < argc; ++ix) {
#ifdef FLEXT_DEBUG
            if(a == a_list && ix > 0) {
                ERRINTERNAL();
            }
#endif
#if FLEXT_SYS == FLEXT_SYS_PD && defined(FLEXT_COMPATIBLE)
            if(a == a_pointer) {
                post("Pointer arguments are not allowed in compatibility mode"); 
            }
#endif
            args[ix] = a;
            a = va_arg(marker,int);
        }
        va_end(marker);
    }
    
    MethItem *mi = new MethItem;
    mi->index = ma->Members();
    mi->SetArgs(fun,argc,args);
    ma->Add(mi,tag,inlet);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::ListMethods(AtomList &la,int inlet) const
{
	typedef TablePtrMap<int,const t_symbol *,32> MethList;
    MethList list[2];
    ItemCont *clmethhead = ClMeths(thisClassId());

    int i;
    for(i = 0; i <= 1; ++i) {
        ItemCont *a = i?methhead:clmethhead;
        if(a && a->Contained(inlet)) {
            ItemSet &ai = a->GetInlet(inlet);
            for(FLEXT_TEMP_TYPENAME ItemSet::iterator as(ai); as; ++as) {
                for(Item *al = as.data(); al; al = al->nxt) {
                    MethItem *aa = (MethItem *)al;
                    // check it's not related to an attribute
                    if(!aa->IsAttr()) {
                        list[i].insert(aa->index,as.key());
                        break;
                    }
                }
            }
        }
    }

    la((int)list[0].size()+(int)list[1].size());
    int ix = 0;
    for(i = 0; i <= 1; ++i)
        for(MethList::iterator it(list[i]); it; ++it) 
            SetSymbol(la[ix++],it.data());
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::cb_ListMethods(flext_base *c,int argc,const t_atom *argv)
{ 
    Locker lock(c);
    if(c->HasAttributes() && (argc == 0 || (argc == 1 && CanbeInt(argv[0])))) {
        // defined in flsupport.cpp
        int inlet = argc?GetAInt(argv[0]):0;
        AtomListStatic<32> la;
        c->ListMethods(la,inlet);
        c->ToOutAnything(c->GetOutAttr(),sym_methods,la.Count(),la.Atoms());
        return true;
    }
    else
        return false;
}

#include "flpopns.h"

#endif // __FLEXT_METH_CPP
