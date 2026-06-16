#!/usr/bin/env python3
import sys

from z3 import And, BitVec, BitVecVal, Solver, sat

OP_LOAD_CHAR, OP_PUSH_CONST, OP_XOR, OP_CMP_EQ = 0xA7, 0x3D, 0xE1, 0x64
OP_AND, OP_RET, OP_JUNK, OP_DEAD = 0x9B, 0x2C, 0xD1, 0x6D
KEY_LEN = 19


def dec(code, i):
    return code[i] ^ ((((i * 17) + 0x5A) ^ 0xA5) & 0xFF)


def solve(code):
    key = [BitVec("key_%02d" % i, 8) for i in range(KEY_LEN)]
    s = Solver()
    stack = []
    ip = 0

    while ip < len(code):
        op = dec(code, ip)
        ip += 1
        if op == OP_LOAD_CHAR:
            stack.append(key[dec(code, ip)])
            ip += 1
        elif op == OP_PUSH_CONST:
            stack.append(BitVecVal(dec(code, ip), 8))
            ip += 1
        elif op == OP_XOR:
            r, l = stack.pop(), stack.pop()
            stack.append(l ^ r)
        elif op == OP_CMP_EQ:
            r, l = stack.pop(), stack.pop()
            stack.append(l == r)
        elif op == OP_AND:
            r, l = stack.pop(), stack.pop()
            stack.append(And(l, r))
        elif op in (OP_JUNK, OP_DEAD):
            ip += 1
        elif op == OP_RET:
            s.add(stack.pop())
            break
        else:
            return None

    if s.check() != sat:
        return None
    m = s.model()
    return "".join(chr(m[k].as_long()) for k in key)


def main(argv):
    if len(argv) != 2:
        print("Usage: %s HEX" % argv[0], file=sys.stderr)
        return 1

    code = bytes.fromhex(argv[1])
    key = solve(code)
    if key is None:
        print("Cannot solve VM bytecode", file=sys.stderr)
        return 1

    print("Bytecode: %d bytes" % len(code))
    print("Z3: SAT")
    print(key)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
