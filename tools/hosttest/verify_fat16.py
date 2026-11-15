#!/usr/bin/env python3
"""Verification independante d'une image FAT16 (superfloppy) : BPB, FAT1==FAT2, chaines, LFN."""
import struct, sys
d = open(sys.argv[1] if len(sys.argv) > 1 else 'disk.img', 'rb').read()
bps, spc, rsvd, nf, rootent, tot16 = struct.unpack_from('<HBHBHH', d, 11)
spf = struct.unpack_from('<H', d, 22)[0]
assert d[510:512] == b'\x55\xaa' and bps == 512 and d[54:62] == b'FAT16   '
rootsecs = (rootent*32 + 511)//512
root = (rsvd + nf*spf)*512
data = root + rootsecs*512
clusters = (tot16 - rsvd - nf*spf - rootsecs)//spc
assert 4085 <= clusters < 65525, clusters
fats = [d[(rsvd+i*spf)*512:(rsvd+(i+1)*spf)*512] for i in range(nf)]
assert all(f == fats[0] for f in fats), 'FAT1 != FAT2'
fat = struct.unpack('<%dH' % (spf*256), fats[0])
assert fat[0] == 0xFFF8 and fat[1] == 0xFFFF
used = set(); entries = []
lfn = {}; lsum = None; lfirst = None
def csum(n):
    s = 0
    for c in n: s = (((s & 1) << 7) + (s >> 1) + c) & 0xFF
    return s
for i in range(rootent):
    e = d[root+i*32: root+i*32+32]
    if e[0] == 0: break
    if e[0] == 0xE5: lfn = {}; continue
    attr = e[11]
    if attr == 0x0F:
        ordv = e[0]
        if ordv & 0x40: lfn = {}; lsum = e[13]
        assert e[13] == lsum
        s = e[1:11] + e[14:26] + e[28:32]
        lfn[ordv & 0x3F] = s.decode('utf-16le'); continue
    if attr & 8: lfn = {}; continue
    name = None
    if lfn:
        assert csum(e[:11]) == lsum, 'checksum LFN faux'
        name = ''.join(lfn[k] for k in sorted(lfn)).split('\0')[0].split('￿')[0]
    lfn = {}
    sn = e[:8].decode('latin1').rstrip() + ('.' + e[8:11].decode('latin1').rstrip() if e[8:11].strip() else '')
    first, size = struct.unpack_from('<H', e, 26)[0], struct.unpack_from('<I', e, 28)[0]
    chain = []; c = first
    while c >= 2 and c < 0xFFF8:
        assert c not in used, 'cluster partage'; used.add(c); chain.append(c); c = fat[c]
        assert len(chain) <= clusters
    assert c >= 0xFFF8 or first == 0, 'chaine non terminee'
    assert len(chain) == (size + spc*512 - 1)//(spc*512), (name or sn, len(chain), size)
    entries.append((name or sn, sn, size, len(chain)))
orphans = [c for c in range(2, clusters+2) if fat[c] != 0 and c not in used]
assert not orphans, 'clusters orphelins: %d' % len(orphans)
for e in entries: print(e)
print('FAT16 OK : %d fichiers, %d clusters utilises/%d' % (len(entries), len(used), clusters))
