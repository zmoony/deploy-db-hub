with open('src/ui/AppStyle.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find raw string start
start = content.find('R"STYLE(')
if start == -1:
    print("No R\"STYLE( found")
else:
    rs_start = start + len('R"STYLE(')
    # Find )STYLE"
    end = content.find(')STYLE"', rs_start)
    if end == -1:
        print("No )STYLE\" found!")
    else:
        raw = content[rs_start:end]
        # Check for STYLE inside
        if 'STYLE' in raw:
            positions = []
            pos = 0
            while True:
                idx = raw.find('STYLE', pos)
                if idx == -1:
                    break
                positions.append(idx)
                pos = idx + 1
            print(f"Found STYLE at {len(positions)} positions inside raw string")
            for p in positions[:10]:
                context = raw[max(0,p-20):p+20]
                print(f"  pos {p}: ...{repr(context)}...")
        else:
            print("No STYLE inside raw string")
