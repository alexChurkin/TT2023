#include "X86.h"
#include "X86InstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define X86_MULADD_PASS_NAME "My custom X86 muladd pass"

namespace {
class X86MulAddPass : public MachineFunctionPass {
public:
    static char ID;

    X86MulAddPass() : MachineFunctionPass(ID) {
        initializeX86MulAddPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    StringRef getPassName() const override { return X86_MULADD_PASS_NAME; }
};

char X86MulAddPass::ID = 0;

bool X86MulAddPass::runOnMachineFunction(MachineFunction &MF) {
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

    for (auto &MBB : MF) {
        for (auto I = MBB.begin(); I != MBB.end(); I++) {
            // Found multiply operation
            if (I->getOpcode() == X86::MULPDrr) {
                auto MulInstr = I;
                auto NextInstr = std::next(MulInstr);
                // And next operation is add
                if (NextInstr->getOpcode() == X86::ADDPDrr) {
                    if (MulInstr->getOperand(0).getReg() == NextInstr->getOperand(1).getReg()) {
                        I--;
                        MachineInstr &MI = *MulInstr;
                        MachineInstrBuilder MIB = BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::VFMADD213PDZ128r));
                        MIB.addReg(NextInstr->getOperand(0).getReg(), RegState::Define);
                        MIB.addReg(MulInstr->getOperand(1).getReg());
                        MIB.addReg(MulInstr->getOperand(2).getReg());
                        MIB.addReg(NextInstr->getOperand(2).getReg());
                        MulInstr->eraseFromParent();
                        NextInstr->eraseFromParent();
                    }
                }
            }
        }
    }

    return false;
}

} // end of anonymous namespace

INITIALIZE_PASS(X86MulAddPass, "x86-muladd",
    X86_MULADD_PASS_NAME,
    false, // is CFG only?
    false  // is analysis?
)

namespace llvm {
    FunctionPass *createX86MulAddPassPass() { return new X86MulAddPass(); }
}
