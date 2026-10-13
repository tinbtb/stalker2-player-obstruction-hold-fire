"""Validate both native hook signatures against a locally supplied game EXE."""
from pathlib import Path
import hashlib,json,re,struct,sys
root=Path(__file__).resolve().parents[1]
data=Path(sys.argv[1]).read_bytes()
meta=json.loads((root/'release.json').read_text())
assert len(data)==174798384 and hashlib.sha256(data).hexdigest()==meta['game_sha256'], 'Unsupported game EXE'
pe=struct.unpack_from('<I',data,0x3c)[0]
assert data[pe:pe+4]==b'PE\0\0'
count=struct.unpack_from('<H',data,pe+6)[0]
optional_size=struct.unpack_from('<H',data,pe+20)[0]
sections=pe+24+optional_size
source=(root/'src/PlayerObstructionHoldFire.c').read_text(encoding='utf-8')
for name,rva,destination in [('expected_hook',0x60c67c,0x44041c),('expected_focus_getter',0x6b6ea52,None)]:
 pattern=re.search(r'\b'+name+r'\[8\]=\{([^}]+)\}',source)
 assert pattern, 'Missing source signature: '+name
 expected=bytes(int(v.strip(),0) for v in pattern.group(1).split(','))
 actual=None
 for i in range(count):
  virtual_size,address,raw_size,raw_pointer=struct.unpack_from('<IIII',data,sections+i*40+8)
  if address<=rva and rva+8<=address+raw_size:
   offset=raw_pointer+rva-address;actual=data[offset:offset+8];break
 assert actual==expected, f'{name}: source={expected.hex()}, executable={actual.hex() if actual else None}'
 if destination is not None:
  assert actual[0]==0xe8 and rva+5+struct.unpack_from('<i',actual,1)[0]==destination, 'Original CALL destination mismatch'
 print(f'{name}: {actual.hex()} at RVA {rva:x} verified')

def at(rva,length):
 for i in range(count):
  _,address,raw_size,raw_pointer=struct.unpack_from('<IIII',data,sections+i*40+8)
  if address<=rva and rva+length<=address+raw_size:
   offset=raw_pointer+rva-address;return data[offset:offset+length]
 raise AssertionError('RVA not backed by file')
image_base=struct.unpack_from('<Q',data,pe+24+24)[0]
name,wrapper=struct.unpack('<QQ',at(0x8c49b70,16))
assert at(name-image_base,len('GetFocusedEnemy')+1)==b'GetFocusedEnemy\0'
assert wrapper-image_base==0x6702046
for call,target in [(0x670209c,0x6b6ea52),(0x6b6eaa8,0x2560e08)]:
 code=at(call,5)
 assert code[0]==0xe8 and call+5+struct.unpack_from('<i',code,1)[0]==target, 'Named focused-enemy getter chain mismatch'
assert 'get_focused_enemy=(void*)(game_base+0x6b6ea52)' in source
print('GetFocusedEnemy registration -> native getter -> weak-actor resolver verified')
