import struct  
import argparse
import re

p = argparse.ArgumentParser(description="Convert a bkread dump file into a format this emulator can read")
p.add_argument('fin', help='Input file')
p.add_argument('fout', help='Input file', default=None, nargs='?')
args = p.parse_args()


with open(args.fin, "rb") as f:  
    bytes = f.read()
# Extract header attributes
address, length, name = struct.unpack("HH16s", bytes[:20])  
print(f"Start address: {address:o} Length: {length} Embedded name: {name}")

format = False
try: 
    format = re.match(r"^\w", name.decode('ascii'))
except UnicodeDecodeError:
    pass

if not format:
    print("This file does not have an embedded name and cannot be converted")
else:
    if not args.fout:
        print("This file should be converted, specify the output file name")
    else:
        # Repackage header without file name
        new_bytes = struct.pack("HH", address, length) + bytes[20:]  
        with open(args.fout, "wb") as f:  
            f.write(new_bytes)
        print(f"Wrote {args.fout}")