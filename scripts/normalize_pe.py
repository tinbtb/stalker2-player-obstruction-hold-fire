"""Remove non-runtime PDB/build identification for cross-directory reproducibility."""
from pathlib import Path
import struct,sys
p=Path(sys.argv[1]);data=bytearray(p.read_bytes())
pe=struct.unpack_from('<I',data,0x3c)[0];optional=pe+24
assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,optional)[0]==0x20b
count=struct.unpack_from('<H',data,pe+6)[0]
table=optional+struct.unpack_from('<H',data,pe+20)[0]
debug_rva,debug_size=struct.unpack_from('<II',data,optional+112+6*8)
for i in range(count):
 at=table+i*40
 if data[at:at+8].rstrip(b'\0')!=b'.buildid':continue
 vsize,rva,rawsize,raw=struct.unpack_from('<IIII',data,at+8)
 flags=struct.unpack_from('<I',data,at+36)[0]
 assert not flags&0x20000000 and rva<=debug_rva and debug_rva+debug_size<=rva+max(vsize,rawsize)
 assert raw+rawsize<=len(data)
 data[raw:raw+rawsize]=b'\0'*rawsize
 struct.pack_into('<II',data,optional+112+6*8,0,0)
 break
else:
 assert debug_rva==0 and debug_size==0
struct.pack_into('<I',data,pe+8,0)
struct.pack_into('<I',data,optional+64,0)
p.write_bytes(data)
