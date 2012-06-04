#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>

#include "llvm.h"


LLVMModuleRef mod;
LLVMBuilderRef builder;


void llvm_createModule()
{
    mod = LLVMModuleCreateWithName("alan_module");
    builder = LLVMCreateBuilder();
}

void llvm_destroyModule()
{
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);
}

void llvm_printModule(char *filename)
{
    LLVMWriteBitcodeToFile(mod, filename);
}
