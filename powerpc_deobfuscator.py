#!/usr/bin/python2

# Deobfuscate PowerPC instructions.
# You're welcome.
#
# Probably full of bugs, use at your own risk.
#
# - plutoo

import sys


def get_operands(operands, pattern):
    ret = []
    i = 0

    if len(operands) != len(pattern):
        print('Expected %d operands.' % len(pattern))
        sys.exit(1)

    try:
        for p in pattern:
            if p == 'i':
                ret.append(int(operands[i]))
            elif p == 's':
                ret.append(operands[i])
            i+=1
    except:
        print('Expected integers as operand #%d.' % i)
        sys.exit(1)

    return ret


# - rlwimi ---------------------------------------------------------------------

def do_rlwimi(insn):
    operands = insn[len('rlwimi '):].replace(' ', '').split(',')
    (ra, rb, n, mb, me) = get_operands(operands, 'ssiii')

    mask = 0
    for i in range(mb, me+1):
        mask = mask | (1 << (32-i-1))

    out = '%s  <-  ' % ra

    if mask^0xffffffff == 0: out += '%s | ' % ra
    else: out += '(%s & ~0x%x) | ' % (ra, mask)

    if mask == 0:
        if n == 0: out += '%s' % rb
        else: out += '(%s << %d)' % (rb, n)
    else:
        if n == 0: out += '(%s & 0x%x)' % (rb, mask)
        else: out += '((%s << %d) & 0x%x)' % (rb, n, mask)

    print(out)

def do_inslwi(insn):
    operands = insn[len('inslwi '):].split(',')
    (ra, rb, n,b) = get_operands(operands, 'ssii')
    do_rlwimi('rlwimi %s,%s,%d,%d,%d' % (ra, rb, 32-b, b, b+n-1))

def do_insrwi(insn):
    operands = insn[len('insrwi '):].split(',')
    (ra, rb, n,b) = get_operands(operands, 'ssii')
    do_rlwimi('rlwimi %s,%s,%d,%d,%d' % (ra, rb, 32-(b+n), b, b+n-1))


# - rlwinm ---------------------------------------------------------------------

def do_rlwinm(insn):
    operands = insn[len('rlwinm '):].replace(' ', '').split(',')
    (ra, rb, n, mb, me) = get_operands(operands, 'ssiii')

    mask = 0
    for i in range(mb, me+1):
        mask = mask | (1 << (32-i-1))

    out = '%s  <-  ' % ra

    if ((1<<n)-1) & mask:
        if n == 0: out += '%s' % rb
        else: out += '(%s << %d)' % (rb, n)

        if mask != 0xffffffff:
            out += ' & 0x%x' % mask
    else:
        out += '(%s & 0x%x) << %d' % (rb, mask>>n, n)

    print(out)

def do_extlwi(insn):
    operands = insn[len('extlwi '):].split(',')
    (ra, rb, n,b) = get_operands(operands, 'ssii')
    do_rlwinm('rlwinm %s,%s,%d,%d,%d' % (ra, rb, b, 0, n-1))

def do_extrwi(insn):
    operands = insn[len('extrwi '):].split(',')
    (ra, rb, n,b) = get_operands(operands, 'ssii')
    do_rlwinm('rlwinm %s,%s,%d,%d,%d' % (ra, rb, b+n, 32-n, 31))

def do_clrlwi(insn):
    operands = insn[len('clrlwi '):].split(',')
    (ra, rb, n) = get_operands(operands, 'ssi')
    do_rlwinm('rlwinm %s,%s,%d,%d,%d' % (ra, rb, 0, n, 31))

def do_clrrwi(insn):
    operands = insn[len('clrrwi '):].split(',')
    (ra, rb, n) = get_operands(operands, 'ssi')
    do_rlwinm('rlwinm %s,%s,%d,%d,%d' % (ra, rb, 0, 0, 31-n))

def do_clrlslwi(insn):
    operands = insn[len('clrlslwi '):].split(',')
    (ra, rb, b,n) = get_operands(operands, 'ssii')
    do_rlwinm('rlwinm %s,%s,%d,%d,%d' % (ra, rb, n, b-n, 31-n))



# - main -----------------------------------------------------------------------

def deobfuscate():
    insn = ' '.join(sys.argv[1:]).replace('  ', ' ').strip()
    mnemonic = insn.split(' ')[0]

    # rlwimi-based
    if mnemonic == 'inslwi':
        do_inslwi(insn)
    elif mnemonic == 'insrwi':
        do_insrwi(insn)
    elif mnemonic == 'rlwimi':
        do_rlwimi(insn)

    # rlwinm-based
    elif mnemonic == 'extlwi':
        do_extlwi(insn)
    elif mnemonic == 'extrwi':
        do_extrwi(insn)
    elif mnemonic == 'clrlwi':
        do_clrlwi(insn)
    elif mnemonic == 'clrrwi':
        do_clrrwi(insn)
    elif mnemonic == 'clrlslwi':
        do_clrlslwi(insn)
    elif mnemonic == 'rlwinm':
        do_rlwinm(insn)

    else:
        print('Sorry dude, can\'t help you. :-(')

deobfuscate()
