#!/usr/bin/python2

# Code diffing tool for 3DS binaries.
#
# Usage:    ./arm32diff.py <code_old.bin> <code_new.bin>
# Install:  sudo pip2 install edit_distance
#
# -- plutoo 2016


import sys, edit_distance

BASE = 0x00100000

def little_endian(x):
    '''
    Converts a string to little endian integer.
    '''
    ret = 0
    n   = 0
    for ch in x:
        ret |= ord(ch) << (8*n)
        n   += 1
    return ret

def get_subs(file_name):
    '''
    Heuristically finds subroutines in ARM code. Not perfect but works well
    enough.
    '''
    buf    = open(file_name, 'rb').read()
    words  = [little_endian(buf[i:i+4]) for i in range(0, len(buf), 4)]
    subs   = set()

    subs.add(0)

    # Find BL instructions.
    pos = 0
    for w in words:
        if w & 0x0F000000 == 0x0B000000:
            offset = w & 0x00FFFFFF
            # Sign extension.
            if offset & (1 << 23):
                offset = offset - (1 << 24)
            sub = pos + (offset << 2) + 8
            if sub > 0:
                subs.add(sub)
        pos += 4

    return subs

def d(x):
    return [x[i+1] - x[i] for i in range(len(x) - 1)]

a = sorted(get_subs(sys.argv[1]))
b = sorted(get_subs(sys.argv[2]))
da = d(a)
db = d(b)

print '[old] Number of functions:', len(a)
print '[new] Number of functions:', len(b)

a = [x + BASE for x in a]
b = [x + BASE for x in b]

for t in edit_distance.SequenceMatcher(a=da, b=db).get_opcodes():
    if t[0] == 'delete':
        print '[old] %08x-%08x was removed (size %04x)' % (a[t[1]], a[t[2]], a[t[2]]-a[t[1]])
    elif t[0] == 'insert':
        print '[new] %08x-%08x was added (size %04x)' % (b[t[3]], b[t[4]], b[t[4]]-b[t[3]])
    elif t[0] == 'replace':
        print '[old] %08x-%08x changed to [new] %08x-%08x' % (a[t[1]], a[t[2]], b[t[3]], b[t[4]])
