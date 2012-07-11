#include <stdio.h>
#include <stdlib.h>
#include <gc.h>
#include <unistd.h>
#include <string.h>
#include "symbol.h"
#include "llvm.h"
#include "quad.h"
#include "general.h"
#include "parser.h"

extern int  yyparse();
extern FILE *yyin;
extern bool global_typeError;

char filename[256];
FILE *outfile;
char inpfile[256];
bool pquads;
char *idx;

void help_exit()
{
    printf( "\n"
            "Usage: ./alan <option> [<optimize_option>] :\n"
            "Reads program from standard input and prints to standard output\n"
            "\n"
            "Usage: ./alan [<optimize_option>] <filename> :\n"
            "Reads program from <filename> and prints to files\n"
            "\n"
            "   option                         Description\n"
            "----------------        ---------------------\n"
            "     -h                show this help\n"
            "     -i                print quads\n"
            "     -l                print llvm code\n"
            "     -f                print assembly code\n"
            "\n"
            " optimize_option                  Description\n"
            "----------------        ---------------------\n"
            "       -O               generate llvm and assebly code at -O3 optimization level\n"
            "       -O0              generate non optimized llvm and assebly code\n"
            "       -O1              generate llvm and assebly code at -O1 optimization level\n"
            "       -O2              generate llvm and assebly code at -O2 optimization level\n"
            "       -O3              generate llvm and assebly code at -O3 optimization level\n"
            "   Examples                        Description\n"
            "----------------        ---------------------\n"
            "./alan -i <file.alan    prints to standard output the quads of file.alan\n"
            "./alan -f -O2 <file     prints assembly code at O2 optimization level\n"
            "./alan -O test.alan     prints quads to test.ism, fully optimized llvm code\n"
            "                        to test.ll and fully optimized assembly code to test.s\n"
            "\n");
    exit(1);
}

int main (int argc, char *argv[])
{
    int ret;
    int c;
    bool i=false,f=false,l=false;
    char buff[256];
    char opt='0';

    GC_INIT();

    initSymbolTable(512);
    llvm_createModule();
    while ((c = getopt (argc, argv, "iflhO::")) != -1) {
        switch (c)
        {
            case 'i':
                if(i||f||l)
                    help_exit(); 
                i=true;
                break;
            case 'f':
                if(i||f||l)
                    help_exit();
                f=true;
                break;
            case 'l':
                if(i||f||l)
                    help_exit();
                l=true;
                break;
            case 'h':
                help_exit();
                break;
            case 'O':
                if(optarg==NULL)
                    opt='3';
                else if((strcmp(optarg,"0"))&&(strcmp(optarg,"1"))&&(strcmp(optarg,"2"))&&(strcmp(optarg,"3"))) 
                    help_exit();
                else 
                    opt=optarg[0];
                break;
            default:
                help_exit();
        }
    }
    strcpy(filename,"stdin");
    if(!i && !f && !l) {
        if((argc-optind)!=1)
            help_exit();
        strcpy(inpfile,argv[optind]);
        strcpy(filename,inpfile);
        if((yyin=fopen(inpfile,"r"))==NULL) 
            help_exit();
        idx=strchrnul(inpfile,'.');
        *idx = '\0';
        sprintf(buff,"%s.ism",inpfile);
        pquads=true;
        outfile = fopen(buff,"w");
        ret = yyparse();
        if((ret!=0)||global_typeError) return ret;
        sprintf(buff,"%s.bc",inpfile);
        llvm_printModule(buff);
        if(opt=='0') {
            sprintf(buff,"llc -o %s.s %s.bc",inpfile,inpfile);
            system(buff);
            sprintf(buff,"llvm-dis -o %s.ll %s.bc",inpfile,inpfile);
            system(buff);
        }
        else {
            sprintf(buff,"opt -O%c -o %s.bc %s.bc",opt,inpfile,inpfile);
            system(buff);
            sprintf(buff,"llc -O%c -o %s.s %s.bc",opt,inpfile,inpfile);
            system(buff);
            sprintf(buff,"llvm-dis -o %s.ll %s.bc",inpfile,inpfile);
            system(buff);
        }
        sprintf(buff,"gcc %s.s lib.s -o %s.out",inpfile,inpfile);
        system(buff); 
    }
    else if(i && !f && !l) {
        if((argc-optind)>0)
           help_exit(); 
        outfile = stdout;
        pquads = true;
        ret = yyparse();
        if((ret!=0)||global_typeError) return ret;
    } else if(f && !i && !l) {
        if((argc-optind)>0)
           help_exit(); 
        pquads = false;
        ret = yyparse();
        if((ret!=0)||global_typeError) return ret;
        llvm_printModule("foo.bc");
        if(opt=='0') {
            system("llc -o - foo.bc");
            system("llc -o foo.s foo.bc");
        }
        else {
            sprintf(buff,"opt -O%c -o foo.bc foo.bc",opt);
            system(buff);
            sprintf(buff,"llc -O%c -o - foo.bc",opt);
            system(buff);
            sprintf(buff,"llc -O%c -o foo.s foo.bc",opt);
            system(buff);
        }
        system("gcc foo.s lib.s -o stdin.out");

    } else if(l && !i && !f) {
        if((argc-optind)>0)
           help_exit(); 
        pquads = false;
        ret = yyparse();
        if((ret!=0)||global_typeError) return ret;
        llvm_printModule("foo.bc");
        if(opt!='0') {
            sprintf(buff,"opt -O%c -o foo.bc foo.bc",opt);
            system(buff);
        }
        system("llvm-dis -o - foo.bc");
    } else help_exit(); 

    llvm_destroyModule();
    destroySymbolTable();
    fclose(yyin);
    return ret;
}

