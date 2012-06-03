#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>


LLVMModuleRef mod;


void llvm_createModule()
{
    mod = LLVMModuleCreateWithName("alan_module");
}
