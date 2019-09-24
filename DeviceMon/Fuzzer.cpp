#include <fltKernel.h>
#include "Fuzzer.h"
#include "include\capstone\capstone.h"
#include "Log.h"

#define DUMMP_FUZZER8_F		0xFF
#define DUMMP_FUZZER16_F	0xFFFF
#define DUMMP_FUZZER32_F	0xFFFFFFFF
#define DUMMP_FUZZER64_F	0xFFFFFFFFFFFFFFFF


#define DUMMP_FUZZER8	(UCHAR)(__rdtsc()^ 0x5A5A5A5A5A5A5A5A)
#define DUMMP_FUZZER16	(USHORT)(__rdtsc()^ 0x5A5A5A5A5A5A5A5A)
#define DUMMP_FUZZER32	(ULONG)(__rdtsc() ^ 0x5A5A5A5A5A5A5A5A)
#define DUMMP_FUZZER64	__rdtsc() ^ 0x5A5A5A5A5A5A5A5A

#define FUZZ_RECORD_LEN 0xFFFF
typedef struct _ACCESS_RECORD
{
	ULONG     DesRegId;
	ULONG_PTR Rip;
}ACCESSRECORD;


ACCESSRECORD Record[FUZZ_RECORD_LEN] = { 0 };


ULONG AccessWidth(
	ULONG RegId
)
{
	ULONG Width = 0;
	switch (RegId)
	{
	case X86_REG_AH:
	case X86_REG_AL:
	case X86_REG_BH:
	case X86_REG_BL:
	case X86_REG_CH:
	case X86_REG_CL:
	case X86_REG_DH:
	case X86_REG_DL:
	case X86_REG_SPL:
	case X86_REG_BPL:
	case X86_REG_SIL:
	case X86_REG_DIL:
	case X86_REG_R8B:
	case X86_REG_R9B:
	case X86_REG_R10B:
	case X86_REG_R11B:
	case X86_REG_R12B:
	case X86_REG_R13B:
	case X86_REG_R14B:
	case X86_REG_R15B:
		Width = sizeof(UCHAR);
	break;

	case X86_REG_AX:
	case X86_REG_BX:
	case X86_REG_CX:
	case X86_REG_DX:
	case X86_REG_SP:
	case X86_REG_BP:
	case X86_REG_SI:
	case X86_REG_DI:
	case X86_REG_R8W:
	case X86_REG_R9W:
	case X86_REG_R10W:
	case X86_REG_R11W:
	case X86_REG_R12W:
	case X86_REG_R13W:
	case X86_REG_R14W:
	case X86_REG_R15W:
		Width = sizeof(USHORT);
	break;

	case X86_REG_EAX:
	case X86_REG_EBX:
	case X86_REG_ECX:
	case X86_REG_EDX:
	case X86_REG_ESI:
	case X86_REG_EDI:
	case X86_REG_ESP:
	case X86_REG_EBP:
	case X86_REG_R8D:
	case X86_REG_R9D:
	case X86_REG_R10D:
	case X86_REG_R11D:
	case X86_REG_R12D:
	case X86_REG_R13D:
	case X86_REG_R14D:
	case X86_REG_R15D:
		Width = sizeof(ULONG);
		break;
	case X86_REG_RAX:
	case X86_REG_RBX:
	case X86_REG_RCX:
	case X86_REG_RDX:
	case X86_REG_RSI:
	case X86_REG_RSP:
	case X86_REG_RBP:
	case X86_REG_RDI:
	case X86_REG_R8:
	case X86_REG_R9:
	case X86_REG_R10:
	case X86_REG_R11:
	case X86_REG_R12:
	case X86_REG_R13:
	case X86_REG_R14:
	case X86_REG_R15:
		Width = sizeof(ULONG64);
		break;
	}
	return Width;

}
void* SelectRegister(
	GpRegisters* Context,
	ULONG		 RegId,
	PULONG		 Width
)
{
	ULONG_PTR* reg = nullptr;
	switch (RegId)
	{
	case X86_REG_AH:
	case X86_REG_AL:
	case X86_REG_AX:
	case X86_REG_EAX:
	case X86_REG_RAX:
		reg = &Context->ax;
	break;

	case X86_REG_BH:
	case X86_REG_BL:
	case X86_REG_BX:
	case X86_REG_EBX:
	case X86_REG_RBX:
		reg = &Context->bx;
		break;

	case X86_REG_CH:
	case X86_REG_CL:
	case X86_REG_CX:
	case X86_REG_ECX:
	case X86_REG_RCX:
		reg = &Context->cx;
	break;
	case X86_REG_DH:
	case X86_REG_DL:
	case X86_REG_DX:
	case X86_REG_EDX:
	case X86_REG_RDX:
		reg = &Context->dx;
	break;

	case X86_REG_SIL:
	case X86_REG_SI:
	case X86_REG_ESI:
	case X86_REG_RSI:
		reg = &Context->si;
	break;

	case X86_REG_DIL:
	case X86_REG_DI:
	case X86_REG_EDI:
	case X86_REG_RDI:
		reg = &Context->di;
	break;

	case X86_REG_R8B:
	case X86_REG_R8W:
	case X86_REG_R8D:
	case X86_REG_R8:
		reg = &Context->r8;
	break;

	case X86_REG_R9B:
	case X86_REG_R9W:
	case X86_REG_R9D:
	case X86_REG_R9:
		reg = &Context->r9;
	break;

	case X86_REG_R10B:
	case X86_REG_R10W:
	case X86_REG_R10D:
	case X86_REG_R10:
		reg = &Context->r10;
	break;

	case X86_REG_R11B:
	case X86_REG_R11W:
	case X86_REG_R11D:
	case X86_REG_R11:
		reg = &Context->r11;
	break;
	
	case X86_REG_R12B:
	case X86_REG_R12W:
	case X86_REG_R12D:
	case X86_REG_R12:
		reg = &Context->r12;
	break;
	
	case X86_REG_R13B:
	case X86_REG_R13W:
	case X86_REG_R13D:
	case X86_REG_R13:
		reg = &Context->r13;
	break;

	case X86_REG_R14B:
	case X86_REG_R14W:
	case X86_REG_R14D:
	case X86_REG_R14:
		reg = &Context->r14;
	break;

	case X86_REG_R15B:
	case X86_REG_R15W:
	case X86_REG_R15D:
	case X86_REG_R15:
		reg = &Context->r15;
	break;
	case X86_REG_BP:
	case X86_REG_EBP:
	case X86_REG_RBP:
		reg = &Context->bp;
	break;

	case X86_REG_SP:
	case X86_REG_ESP:
	case X86_REG_RSP:
		reg = &Context->sp;
	break;
	default:
		break;
	}

	*Width = AccessWidth(RegId);
	
	return reg;
}

ULONG GetInstructionDestRegister(
	GpRegisters*	Context,
	ULONG_PTR		InstPointer,
	ULONG			InstLen
)
{
	ULONG DestRegId = 0;
	cs_insn* instructions = nullptr;
	cs_regs regs_read, regs_write;
	uint8_t read_count, write_count, i;

	/// Save floating point state
	KFLOATING_SAVE float_save = {};
	auto status = KeSaveFloatingPointState(&float_save);
	if (!NT_SUCCESS(status)) {
		return DestRegId;
	}

	// Disassemble at most 15 bytes to get an instruction size
	csh handle = {};
	const auto mode = CS_MODE_64;
	if (cs_open(CS_ARCH_X86, mode, &handle) != CS_ERR_OK) {
		KeRestoreFloatingPointState(&float_save);
		return DestRegId;
	}

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	const auto count =
		cs_disasm(handle, reinterpret_cast<uint8_t*>(InstPointer), InstLen,
		(ULONG64)(InstPointer), 1, &instructions);

	if (count == 0) {
		cs_close(&handle);
		KeRestoreFloatingPointState(&float_save);
		return DestRegId;
	}

	HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Extracting Instruction= %p  %s \r\n", InstPointer, instructions[0].op_str);

	if (cs_regs_access(handle, &instructions[0],
		regs_read, &read_count,
		regs_write, &write_count) != 0)
	{
		cs_free(instructions, count);
		cs_close(&handle);
		KeRestoreFloatingPointState(&float_save);
		return DestRegId;
	}

	if (!write_count)
	{
		cs_free(instructions, count);
		cs_close(&handle);
		KeRestoreFloatingPointState(&float_save);
		return DestRegId;
	}

	for (i = 0; i < write_count; i++)
	{
		ULONG width = 0;
		char* reg = (char*)SelectRegister(Context, regs_write[i], &width);
		if (!reg)
		{
			continue;
		}
		DestRegId = regs_write[i];
		break;
	}

	cs_free(instructions, count);
	cs_close(&handle);
	KeRestoreFloatingPointState(&float_save);

	return DestRegId;
}
ULONG GetRegIndexByRip(
	GpRegisters*  Context,
	ULONG_PTR InstPointer,
	ULONG		  InstLen
)
{
	ULONG Index = UtilPfnFromVa((void*)InstPointer) % FUZZ_RECORD_LEN;
	ULONG count = 0;

	//If RIP firstly present 
	if (!Record[Index].Rip)
	{
		Record[Index].Rip		= InstPointer;
		Record[Index].DesRegId  = GetInstructionDestRegister(
			Context,
			InstPointer,
			InstLen
		);
		return Record[Index].DesRegId;
	}
	//If already exist
	else if (Record[Index].Rip == InstPointer)
	{
		return Record[Index].DesRegId;
	}

	//If hash collision happened fallback
	//Find if already moved once
	count = 0;
	while (Record[Index].Rip != InstPointer && count < FUZZ_RECORD_LEN){
		Index = ((Index + 1) % FUZZ_RECORD_LEN);
		count++;
	}
	//return if found moved one
	if (count < FUZZ_RECORD_LEN)
	{
		return Record[Index].DesRegId;
	}
	
	//cannot find that, it should be moved to other place
	//find a empty slot
	Index = UtilPfnFromVa((void*)InstPointer) % FUZZ_RECORD_LEN;
	count = 0;
	while (Record[Index].Rip && count < FUZZ_RECORD_LEN){
		Index = ((Index + 1) % FUZZ_RECORD_LEN);
		count++;
	}

	//cannot find any free slot
	NT_ASSERT(Index < FUZZ_RECORD_LEN);

	//Found a free slot, moving the collision one to other place.
	Record[Index].Rip		= InstPointer;
	Record[Index].DesRegId  = GetInstructionDestRegister(
		Context,
		InstPointer,
		InstLen
	); 

 	return Record[Index].DesRegId;
}

bool StartFuzz(
	GpRegisters*  Context,
	ULONG_PTR InstPointer,
	ULONG_PTR MmioAddress,
	ULONG		  InstLen,
	FUZZ*		  fuzzer
)
{
	bool ret = false;
	ULONG width = 0;
	ULONG RegId = GetRegIndexByRip(Context, InstPointer, InstLen);
	if (!RegId)
	{
		return ret;
	}
	
	char* reg = (char*)SelectRegister(Context, RegId, &width);
	if (!reg)
	{
		return ret;
	}
			
	switch (width)
	{
	case 1:
		//Don't fuzz on 8bit access. '/
		//HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Before: %d reg= %p", RegId, *(ULONG_PTR*)reg);
		//*(UCHAR*)reg |= DUMMP_FUZZER8;
		//HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] After: %d reg= %p", RegId, *(ULONG_PTR*)reg);
		//ret = true;
		break;
	case 2:
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Word Before: %d reg= %x", RegId, *(USHORT*)reg);
		//*(USHORT*)reg |= DUMMP_FUZZER16_F;
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Word After: %d reg= %x", RegId, *(USHORT*)reg);
		//ret = true;
		break;
	case 4:
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Dword Before: %d reg= %x", RegId, *(ULONG*)reg);
		//*(ULONG*)reg |= DUMMP_FUZZER32_F;
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Dword After: %d reg= %x", RegId, *(ULONG*)reg);
		//ret = true;
		break;
	case 8:
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Qword Before: %d reg= %p", RegId, *(ULONG_PTR*)reg);
		//*(ULONG64*)reg |= DUMMP_FUZZER64_F;
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Fuzz] Qword After: %d reg= %p",  RegId, *(ULONG_PTR*)reg);
		//ret = true;
		break;
	default:
			break;
	}

	return ret;
}