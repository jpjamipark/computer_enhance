#include <stdio.h>
#include <string.h>
#include "sim86_shared.h"

u32 getFileSize(FILE *f)
{
    fseek(f, 0L, SEEK_END);
    u32 sz = ftell(f);
    rewind(f);
    return sz;
}

void Mov(instruction *ins, u16 *reg, u8 *mem)
{
    switch (ins->Operands[1].Type)
    {
    case operand_type::Operand_Register:
        reg[ins->Operands[0].Register.Index] = reg[ins->Operands[1].Register.Index];
        break;
    case operand_type::Operand_Immediate:
        reg[ins->Operands[0].Register.Index] = ins->Operands[1].Immediate.Value;
        break;
    case operand_type::Operand_Memory:
        printf("exp: %x", ins->Operands[0].Address.ExplicitSegment);
        break;
    default:
        break;
    }
}

void Sub(instruction *ins, u16 *mem, u16 *flags, bool updateMem)
{
    u16 val = 0;
    switch (ins->Operands[1].Type)
    {
    case operand_type::Operand_Register:
        val = mem[ins->Operands[0].Register.Index] - mem[ins->Operands[1].Register.Index];
        break;
    case operand_type::Operand_Immediate:
        val = mem[ins->Operands[0].Register.Index] - ins->Operands[1].Immediate.Value;
        break;
    default:
        break;
    }

    if (updateMem)
    {
        mem[ins->Operands[0].Register.Index] = val;
    }

    u32 signMask = 0x0080;
    u32 bitvalue = val & 0x8000;
    if (bitvalue == 0)
    {
        *flags &= ~signMask;
    }
    else
    {
        *flags |= signMask;
    }

    u32 zeroMask = 0x0040;
    if (val != 0)
    {
        *flags &= ~zeroMask;
    }
    else
    {
        *flags |= zeroMask;
    }
}

void Add(instruction *ins, u16 *mem, u16 *flags)
{
    switch (ins->Operands[1].Type)
    {
    case operand_type::Operand_Register:
        mem[ins->Operands[0].Register.Index] += mem[ins->Operands[1].Register.Index];
        break;
    case operand_type::Operand_Immediate:
        mem[ins->Operands[0].Register.Index] += ins->Operands[1].Immediate.Value;
        break;
    default:
        break;
    }

    u32 signMask = 0x0080;
    u32 bitvalue = mem[ins->Operands[0].Register.Index] & 0x8000;
    if (bitvalue == 0)
    {
        *flags &= ~signMask;
    }
    else
    {
        *flags |= signMask;
    }

    u32 zeroMask = 0x0040;
    if (mem[ins->Operands[0].Register.Index] != 0)
    {
        *flags &= ~zeroMask;
    }
    else
    {
        *flags |= zeroMask;
    }
}

void Jne(instruction *ins, u16 *mem, u16 *flags, u16 *ip) {
    // printf("Immediate: %x\n", ins->Operands[0].Immediate);
    if (!(*flags & 0x0040)) {
        *ip = *ip + ins->Operands[0].Immediate.Value;
        // printf("IP + Immediate: %x\n", *ip + ins->Operands[0].Immediate.Value);
    }
}

void LogEffectiveAddress(effective_address_expression address) {
    printf(" [");
    if (address.Terms[0].Register.Index) {
        printf("%s", Sim86_RegisterNameFromOperand(&address.Terms[0].Register));
        if (address.Terms[1].Register.Index) {
            printf("+%s", Sim86_RegisterNameFromOperand(&address.Terms[1].Register));
        }
    }

    if(address.Displacement) {
       printf("+%d", address.Displacement);
    }
    printf("] ");
}

void PrintInstruction(instruction ins)
{

    printf("%s ", Sim86_MnemonicFromOperationType(ins.Op));

    switch (ins.Operands[0].Type)
    {
    case operand_type::Operand_Register:
        printf("%s ", Sim86_RegisterNameFromOperand(&ins.Operands[0].Register));
        break;
    case operand_type::Operand_Immediate:
        printf("%x ", ins.Operands[0].Immediate);
        break;
    case operand_type::Operand_Memory:
        LogEffectiveAddress(ins.Operands[0].Address);
        break;
    default:
        break;
    }

    switch (ins.Operands[1].Type)
    {
    case operand_type::Operand_Register:
        printf("%s ; ", Sim86_RegisterNameFromOperand(&ins.Operands[1].Register));
        break;
    case operand_type::Operand_Immediate:
        printf("%x ; ", ins.Operands[1].Immediate);
        break;
    case operand_type::Operand_Memory:
        LogEffectiveAddress(ins.Operands[1].Address);
        break;
    default:
        break;
    }
}

void PrintRegisters(u16 *mem)
{
    for (u32 i = 1; i < 9; ++i)
    {
        register_access Reg{.Index = i, .Count = 2, .Offset = 2};
        printf("%s: %x\n", Sim86_RegisterNameFromOperand(&Reg), mem[i]);
    }
}

void PrintFlags(u16 flags)
{
    printf("Flags: ");
    if (flags & 0x80)
    {
        printf("SF: %x, ", (flags & 0x80) >> 6);
    }
    if (flags & 0x40)
    {
        printf("ZF: %x", (flags & 0x40) >> 6);
    }
    printf("\n");
}

struct simulation_state
{
    u16 Registers[9];
    u8 Memory[1024 * 1024];
    u16 Flags = 0;
    u16 InstructionPointer = 0;
};

simulation_state InitialState()
{
    simulation_state s = {};

    for (u32 i = 1; i < 9; ++i)
    {
        s.Registers[i] = 0;
    }

    for (u32 i = 0; i < 1024 * 1024; ++i)
    {
        s.Memory[i] = 0;
    }

    return s;
}

void diffState(simulation_state s1, simulation_state s2)
{
    for (u32 i = 0; i < 9; ++i)
    {
        if (s1.Registers[i] != s2.Registers[i])
        {
            register_access Reg{.Index = i, .Count = 2, .Offset = 2};
            printf("%s: %x -> %x, ", Sim86_RegisterNameFromOperand(&Reg), s1.Registers[i], s2.Registers[i]);
        }
    }

    if (s1.Flags != s2.Flags)
    {
        if ((s1.Flags & 0x80) != (s2.Flags & 0x80))
        {

            printf("SF: %x -> %x, ", (s1.Flags & 0x80) >> 7, (s2.Flags & 0x80) >> 7);
        }
        if ((s1.Flags & 0x40) != (s2.Flags & 0x40))
        {
            printf("ZF: %x -> %x, ", (s1.Flags & 0x40) >> 6, (s2.Flags & 0x40) >> 6);
        }
    }

    if (s1.InstructionPointer != s2.InstructionPointer)
    {
        printf("IP: %x -> %x", s1.InstructionPointer, s2.InstructionPointer);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: ./memory.o <filename>\n");
        return 1;
    }

    const char *filename = argv[1];
    FILE *binary = fopen(filename, "rb");
    if (binary == nullptr)
    {
        printf("Failed to open file: %s\n", filename);
        return 1;
    }

    u32 size = getFileSize(binary);
    u8 byte_arr[size];
    for (int i = 0; i < size; ++i)
    {
        byte_arr[i] = fgetc(binary);
    }

    u32 Version = Sim86_GetVersion();
    printf("Sim86 Version: %u (expected %u)\n", Version, SIM86_VERSION);
    if (Version != SIM86_VERSION)
    {
        printf("ERROR: Header file version doesn't match DLL.\n");
        return -1;
    }

    simulation_state ss = InitialState();

    while (ss.InstructionPointer < sizeof(byte_arr))
    {
        instruction Decoded;
        Sim86_Decode8086Instruction(sizeof(byte_arr) - ss.InstructionPointer, byte_arr + ss.InstructionPointer, &Decoded);

        if (Decoded.Op)
        {
            PrintInstruction(Decoded);
            simulation_state oldState = ss;
            ss.InstructionPointer += Decoded.Size;
            switch (Decoded.Op)
            {
            case Op_mov:
                Mov(&Decoded, ss.Registers, ss.Memory);
                break;
            case Op_sub:
                Sub(&Decoded, ss.Registers, &ss.Flags, true);
                break;
            case Op_cmp:
                Sub(&Decoded, ss.Registers, &ss.Flags, false);
                break;
            case Op_add:
                Add(&Decoded, ss.Registers, &ss.Flags);
                break;
            case Op_jne:
                Jne(&Decoded, ss.Registers, &ss.Flags, &ss.InstructionPointer);
                break;
            default:
                printf("Not implemented yet!");
                break;
            }
            // PrintFlags(ss.Flags);
            diffState(oldState, ss);
            printf("\n");
        }
        else
        {
            printf("Unrecognized instruction\n");
            break;
        }
    }
    printf("\nRegisters:\n");
    PrintRegisters(ss.Registers);
    PrintFlags(ss.Flags);
    printf("IP: %x\n", ss.InstructionPointer);

    fclose(binary);
    return 0;
}
