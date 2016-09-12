// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef __TIANSHANICE_OBJ_LOCATOR_ICE__
#define __TIANSHANICE_OBJ_LOCATOR_ICE__

// $(ICE_ROOT)\bin\slice2cpp.exe -I$(ICE_ROOT)/slice -I$(ZQProjsPath)/tianshan/ice --output-dir $(ZQPROJSPATH)\tianshan\AccreditedPath\  $(ZQPROJSPATH)/tianshan/AccreditedPath/$(InputName).ice

#include <Ice/ServantLocator.ice>

module TianShanIce
{

module ObjectDB
{

local interface ObjLocator extends Ice::ServantLocator
{

// clone of interface Freeze::Evictor

};

}; }; // namespaces

#endif // __TIANSHANICE_OBJ_LOCATOR_ICE__
