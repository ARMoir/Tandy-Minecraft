"""Convert renderer PPMs to native 320x200 and CRT-aspect PNGs (stdlib only)."""
from pathlib import Path
import struct,zlib
def chunk(t,b):return struct.pack('>I',len(b))+t+b+struct.pack('>I',zlib.crc32(t+b))
def png(path,b,w,h):
    raw=b''.join(b'\0'+b[y*w*3:(y+1)*w*3] for y in range(h))
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
for name in ('before','after','hybrid','blocks','doorway'):
    b=Path(name+'.ppm').read_bytes().split(b'\n',3)[3]
    png(name+'-native.png',b,320,200)
    rows=[]
    for y in range(600):
        row=b[(y//3)*960:(y//3+1)*960]
        rows.append(b''.join(row[x:x+3]*3 for x in range(0,960,3)))
    png(name+'.png',b''.join(rows),960,600)
