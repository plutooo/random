#!/usr/bin/python2

# Deobfuscate PowerPC instructions.
# You're welcome.
#
# Probably full of bugs, use at your own risk.
#
# - plutoo

import sys


DEBUG=1

def get_operands(operands, pattern):
    ret = []
    i = 0

    if len(operands) != len(pattern):
        print('Expected %d operands.' % len(pattern))
        sys.exit(1)

    try:
        for p in pattern:
            if p == 'i':
                if operands[i].startswith('0x'):
                    ret.append(int(operands[i][2:], 16))
                else:
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
    raise Exception('Not implemented.')

def do_inslwi(insn):
    operands = insn[len('inslwi '):].split(',')
    (ra, rb, n,b) = get_operands(operands, 'ssii')
    do_rlwimi('rlwimi %s,%s,%d,%d,%d' % (ra, rb, 32-b, b, b+n-1))

def do_insrwi(insn):
    operands = insn[len('insrwi '):].split(',')
    (ra, rb, n,b) = get_operands(operands, 'ssii')
    do_rlwimi('rlwimi %s,%s,%d,%d,%d' % (ra, rb, 32-(b+n), b, b+n-1))


# - rlwinm ---------------------------------------------------------------------

ONES = lambda n: (1<<n)-1
ROL32 = lambda x, n: (x << n) | (x >> (32-n))
ROR32 = lambda x, n: (x >> n) | (x << (32-n))

def do_rlwinm(insn):
    operands = insn[len('rlwinm '):].replace(' ', '').split(',')
    (ra, rb, n, mb, me) = get_operands(operands, 'ssiii')

    rot = 31-me
    if mb > me:
        me += 32

    #if rot < 0:
    #    mask = ROR32(ONES(me-mb+1), -rot)
    #else:
    mask = ROL32(ONES(me-mb+1), rot)

    left_shift_terms = []
    all_ones = ONES(32-n) << n
    if all_ones & mask:
        if n == 0:
            if mask & all_ones != all_ones:
                left_shift_terms.append('%s & 0x%x' % (rb, mask & all_ones))
            else:
                left_shift_terms.append('%s' % rb)
        else:
            if mask & all_ones != all_ones:
                left_shift_terms.append('(%s << %d) & 0x%x' %
                    (rb, n, mask & all_ones))
                left_shift_terms.append('(%s & 0x%x) << %d' %
                    (rb, (mask & all_ones) >> n, n))
            else:
                left_shift_terms.append('%s << %d' % (rb, n))

    right_shift_terms = []
    all_ones = ONES(n)
    if n != 0 and (all_ones & mask):
        if mask & all_ones != all_ones:
            right_shift_terms.append('(%s >> %d) & 0x%x' %
                (rb, 32-n, mask & all_ones))
            right_shift_terms.append('(%s & 0x%x) >> %d' %
                (rb, (mask & all_ones) << (32-n), 32-n))
        else:
            right_shift_terms.append('%s >> %d' % (rb, 32-n))

    print 'Possible simplifications:'
    for lt in left_shift_terms:
        for rt in right_shift_terms:
            print '%s  <-  %s | %s' % (ra, lt, rt)
    if left_shift_terms == []:
        for rt in right_shift_terms:
            print '%s  <-  %s' % (ra, rt)
    if right_shift_terms == []:
        for lt in left_shift_terms:
            print '%s  <-  %s' % (ra, lt)

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
