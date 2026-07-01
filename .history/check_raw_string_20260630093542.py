with open('src/ui/AppStyle.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find R"STYLE( start
start = content.find('R"STYLE(')
if start == -1:
    print("No R\"STYLE( found")
else:
    rs_start = start + len('R"STYLE(')
    print(f"R\"STYLE( starts at position {rs_start}")
    
    # Find )STYLE"
    end = content.find(')STYLE"', rs_start)
    if end == -1:
        print("No )STYLE\" found!")
    else:
        line_num = content[:end].count('\n') + 1
        print(f")STYLE\" found at position {end}, line {line_num}")
        print(f"Context: ...{repr(content[end-30:end+20])}...")
