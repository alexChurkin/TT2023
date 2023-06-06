#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;
 
namespace {
 
class MyASTConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  std::set<std::string> ParsedTemplates;
 
public:
  MyASTConsumer(CompilerInstance &Instance,
                         std::set<std::string> ParsedTemplates)
      : Instance(Instance), ParsedTemplates(ParsedTemplates) {}
 
  void HandleTranslationUnit(ASTContext& context) override {
    if (!Instance.getLangOpts().DelayedTemplateParsing)
      return;
 
    // This demonstrates how to force instantiation of some templates in
    // -fdelayed-template-parsing mode. (Note: Doing this unconditionally for
    // all templates is similar to not using -fdelayed-template-parsig in the
    // first place.)
    // The advantage of doing this in HandleTranslationUnit() is that all
    // codegen (when using -add-plugin) is completely finished and this can't
    // affect the compiler output.
    struct Visitor : public RecursiveASTVisitor<Visitor> {
      const std::set<std::string> &ParsedTemplates;
      Visitor(const std::set<std::string> &ParsedTemplates)
          : ParsedTemplates(ParsedTemplates) {}
 
      bool VisitCXXRecordDecl(CXXRecordDecl *CxxRDecl) {
        if (CxxRDecl->isClass() || CxxRDecl->isStruct()) {
          llvm::errs() << CxxRDecl->getNameAsString() << "\n";
 
          for (auto it = CxxRDecl->decls_begin(); it != CxxRDecl->decls_end(); ++it)
            if (FieldDecl* nDecl = dyn_cast<FieldDecl>(*it))
              llvm::errs() << "|_ " << nDecl->getNameAsString() << "\n";
 
          llvm::errs() << "\n";
 
        }
        return true;
      }
    } v(ParsedTemplates);
    v.TraverseDecl(context.getTranslationUnitDecl());
  }
};
 
class PrintFunctionNamesAction : public PluginASTAction {
  std::set<std::string> ParsedTemplates;
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::make_unique<MyASTConsumer>(CI, ParsedTemplates);
  }
 
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};
 
}
 
static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X("classprinter", "prints names of classes and their fields");