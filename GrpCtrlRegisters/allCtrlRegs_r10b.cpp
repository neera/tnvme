#include "allCtrlRegs_r10b.h"
#include "../globals.h"


AllCtrlRegs_r10b::AllCtrlRegs_r10b(int fd) : Test(fd)
{
    // 66 chars allowed:     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    mTestDesc.SetCompliance("revision 1.0b, section 3");
    mTestDesc.SetShort(     "Validate all controller registers syntactically");
    // No string size limit for the long description
    mTestDesc.SetLong(
        "Validates the following; the RO fields which are not implementation "
        "specific contain default values; The RO fields cannot be written; All "
        "ASCII fields only contain chars 0x20 to 0x7e.");
}


AllCtrlRegs_r10b::~AllCtrlRegs_r10b()
{
}


bool
AllCtrlRegs_r10b::RunCoreTest()
{
    if (ValidateDefaultValues() == false)
        return false;
    else if (ValidateROBitsAfterWriting() == false)
        return false;
    return true;
}


bool
AllCtrlRegs_r10b::ValidateDefaultValues()
{
    const CtlSpcType *ctlMetrics = gRegisters->GetCtlMetrics();

    LOG_NRM("Validating default register values");

    // Traverse the ctrl'r registers
    for (int j = 0; j < CTLSPC_FENCE; j++) {
        if (ctlMetrics[j].specRev != SPECREV_10b)
            continue;

        if (ValidateCtlRegisterROAttribute((CtlSpc)j) == false)
            return false;
    }

    return true;
}


bool
AllCtrlRegs_r10b::ValidateROBitsAfterWriting()
{
    ULONGLONG value;
    ULONGLONG origValue;
    const CtlSpcType *ctlMetrics = gRegisters->GetCtlMetrics();

    LOG_NRM("Validating RO bits after writing");

    /// Traverse the ctrl'r registers
    for (int j = 0; j < CTLSPC_FENCE; j++) {
        if (ctlMetrics[j].specRev != SPECREV_10b)
            continue;

        // Reserved areas at NOT suppose to be written
        if ((j == CTLSPC_RES1) || (j == CTLSPC_RES2) || (j == CTLSPC_RES3))
            continue;

        LOG_NRM("Validate RO attribute after trying to write 1");
        if (gRegisters->Read((CtlSpc)j, origValue) == false)
            return false;
        value = (origValue | ctlMetrics[j].maskRO);
        if (gRegisters->Write((CtlSpc)j, value) == false)
            return false;
        if (ValidateCtlRegisterROAttribute((CtlSpc)j) == false)
            return false;

        LOG_NRM("Validate RO attribute after trying to write 0");
        value = (origValue & ~ctlMetrics[j].maskRO);
        if (gRegisters->Write((CtlSpc)j, value) == false)
            return false;
        if (ValidateCtlRegisterROAttribute((CtlSpc)j) == false)
            return false;
    }


    return true;
}


int
AllCtrlRegs_r10b::ReportOffendingBitPos(ULONGLONG val, ULONGLONG expectedVal)
{
    ULONGLONG bitMask;

    for (int i = 0; i < (int)(sizeof(ULONGLONG)*8); i++) {
        bitMask = (1 << i);
        if ((val & bitMask) != (expectedVal & bitMask))
            return i;
    }
    return INT_MAX; // there is no mismatch
}


bool
AllCtrlRegs_r10b::ValidateCtlRegisterROAttribute(CtlSpc reg)
{
    ULONGLONG value;
    ULONGLONG expectedValue;
    const CtlSpcType *ctlMetrics = gRegisters->GetCtlMetrics();

    if (ctlMetrics[reg].size > MAX_SUPPORTED_REG_SIZE) {
        for (int k = 0; (k*sizeof(value)) < ctlMetrics[reg].size; k++) {

            if (gRegisters->Read(NVMEIO_BAR01, sizeof(value),
                ctlMetrics[reg].offset + (k * sizeof(value)),
                (unsigned char *)&value) == false) {

                return false;
            } else {
                // Ignore the implementation specific bits, and bits that the
                // manufacturer can make a decision as to their type of access
                value &= ~ctlMetrics[reg].impSpec;

                // Verify that the RO bits are set to correct default values, no
                // reset needed to achieve this because there's no way to change.
                value &= ctlMetrics[reg].maskRO;
                expectedValue =
                    (ctlMetrics[reg].dfltValue & ctlMetrics[reg].maskRO);

                if (value != expectedValue) {
                    LOG_ERR("%s RO bit #%d has incorrect value",
                        ctlMetrics[reg].desc,
                        ReportOffendingBitPos(value, expectedValue));
                    return false;
                }
            }
        }
    } else if (gRegisters->Read(reg, value) == false) {
        return false;
    } else {
        // Ignore the implementation specific bits, and bits that the
        // manufacturer can make a decision as to their type of access RW/RO
        value &= ~ctlMetrics[reg].impSpec;

        // Verify that the RO bits are set to correct default values, no
        // reset needed to achieve this because there's no way to change.
        value &= ctlMetrics[reg].maskRO;
        expectedValue = (ctlMetrics[reg].dfltValue & ctlMetrics[reg].maskRO);

        if (value != expectedValue) {
            LOG_ERR("%s RO bit #%d has incorrect value",
                ctlMetrics[reg].desc,
                ReportOffendingBitPos(value, expectedValue));
            return false;
        }
    }

    return true;
}


