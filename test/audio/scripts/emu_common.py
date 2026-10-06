"""Shared helpers for running original Black & White x86 code under Unicorn (pip install unicorn)."""
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBP,
    UC_X86_REG_GDTR, UC_X86_REG_FS, UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES,
)

RETURN_MAGIC = 0x0FFF0000
TEB_BASE = 0x0FFE0000
GDT_BASE = 0x0FFD0000


def gdt_entry(base, limit, access, flags):
    entry = limit & 0xFFFF
    entry |= (base & 0xFFFFFF) << 16
    entry |= (access & 0xFF) << 40
    entry |= ((limit >> 16) & 0xF) << 48
    entry |= (flags & 0xF) << 52
    entry |= ((base >> 24) & 0xFF) << 56
    return struct.pack("<Q", entry)
STACK_BASE = 0x00100000
STACK_SIZE = 0x00100000
HEAP_BASE = 0x30000000
HEAP_SIZE = 0x04000000


def align(value, alignment=0x1000):
    return (value + alignment - 1) & ~(alignment - 1)


class Emulator:
    def __init__(self, pe_path):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        data = open(pe_path, "rb").read()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        self.image_base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        image_size = struct.unpack_from("<I", data, pe + 24 + 56)[0]
        self.uc.mem_map(self.image_base, align(image_size))
        headers_size = struct.unpack_from("<I", data, pe + 24 + 60)[0]
        self.uc.mem_write(self.image_base, data[:headers_size])
        for i in range(count):
            o = pe + 24 + optional + 40 * i
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", data, o + 8)
            raw = data[rptr:rptr + min(rsize, vsize)]
            self.uc.mem_write(self.image_base + va, raw)
        self.uc.mem_map(STACK_BASE, STACK_SIZE)
        self.uc.mem_map(HEAP_BASE, HEAP_SIZE)
        self.uc.mem_map(RETURN_MAGIC, 0x1000)
        # Thread information block for SEH frames (fs:[0])
        self.uc.mem_map(TEB_BASE, 0x1000)
        self.uc.mem_write(TEB_BASE, struct.pack("<I", 0xFFFFFFFF))
        self.uc.mem_map(GDT_BASE, 0x1000)
        gdt = gdt_entry(0, 0, 0, 0) + gdt_entry(TEB_BASE, 0xFFF, 0x92, 0x4) + gdt_entry(0, 0xFFFFF, 0x92, 0xC)
        self.uc.mem_write(GDT_BASE, gdt)
        self.uc.reg_write(UC_X86_REG_GDTR, (0, GDT_BASE, len(gdt) - 1, 0))
        self.uc.reg_write(UC_X86_REG_FS, 1 << 3)
        for segment in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES):
            self.uc.reg_write(segment, 2 << 3)
        self.heap = HEAP_BASE
        self.stubs = {}
        self.uc.hook_add(UC_HOOK_CODE, self._on_code)
        self.uc.hook_add(UC_HOOK_MEM_INVALID, self._on_invalid)

    def _on_invalid(self, uc, access, address, size, value, user):
        eip = uc.reg_read(UC_X86_REG_EIP)
        raise RuntimeError(f"invalid memory access {access} at {address:#x} from {eip:#x}")

    def _on_code(self, uc, address, size, user):
        stub = self.stubs.get(address)
        if stub is None:
            return
        handler, arg_bytes = stub
        esp = uc.reg_read(UC_X86_REG_ESP)
        ret = self.u32(esp)
        result = handler(self)
        uc.reg_write(UC_X86_REG_EAX, (result or 0) & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP, esp + 4 + arg_bytes)
        uc.reg_write(UC_X86_REG_EIP, ret)

    def stub(self, address, handler, arg_bytes=0):
        """Replace the function at address. arg_bytes is what the callee pops (0 for cdecl)."""
        self.stubs[address] = (handler, arg_bytes)

    # Memory helpers
    def alloc(self, size, alignment=8):
        self.heap = align(self.heap, alignment)
        address = self.heap
        self.heap += max(size, 1)
        self.uc.mem_write(address, b"\0" * size)
        return address

    def u32(self, address):
        return struct.unpack("<I", self.uc.mem_read(address, 4))[0]

    def i32(self, address):
        return struct.unpack("<i", self.uc.mem_read(address, 4))[0]

    def u16(self, address):
        return struct.unpack("<H", self.uc.mem_read(address, 2))[0]

    def f32_bits(self, address):
        return self.u32(address)

    def f32(self, address):
        return struct.unpack("<f", self.uc.mem_read(address, 4))[0]

    def w32(self, address, value):
        self.uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))

    def wf32(self, address, value):
        self.uc.mem_write(address, struct.pack("<f", value))

    def write(self, address, data):
        self.uc.mem_write(address, data)

    def arg(self, index):
        """Stack argument of the stubbed function currently being handled (0 based)."""
        esp = self.uc.reg_read(UC_X86_REG_ESP)
        return self.u32(esp + 4 + 4 * index)

    def ecx(self):
        return self.uc.reg_read(UC_X86_REG_ECX)

    def call(self, address, args=(), ecx=0, edx=0):
        esp = STACK_BASE + STACK_SIZE - 0x100
        for value in reversed(args):
            esp -= 4
            self.w32(esp, value)
        esp -= 4
        self.w32(esp, RETURN_MAGIC)
        self.uc.reg_write(UC_X86_REG_ESP, esp)
        self.uc.reg_write(UC_X86_REG_EBP, 0)
        self.uc.reg_write(UC_X86_REG_ECX, ecx)
        self.uc.reg_write(UC_X86_REG_EDX, edx)
        try:
            self.uc.emu_start(address, RETURN_MAGIC)
        except Exception as error:
            eip = self.uc.reg_read(UC_X86_REG_EIP)
            raise RuntimeError(f"emulation failed in call to {address:#x} at {eip:#x}: {error}") from error
        return self.uc.reg_read(UC_X86_REG_EAX)


class MsvcRand:
    """LIBCMT rand/srand."""

    def __init__(self):
        self.state = 1

    def seed(self, value):
        self.state = value & 0xFFFFFFFF

    def next(self):
        self.state = (self.state * 214013 + 2531011) & 0xFFFFFFFF
        return (self.state >> 16) & 0x7FFF
