def IsSpace(ch : str): return ch == ' ' or ch == '\t'
if __name__ == "__main__":
    output_file_path = input("Input the output file name: ")
    output_file = open(output_file_path, 'w')
    
    print("Now Input the enum content ([q]: quit)")
    while True:
        s = input()
        if s == '[q]': break
        # ignore the prefix
        for i in range(0, len(s)):
            if not IsSpace(s[i]):
                s = s[i: ]
                break
        # ignore the hint
        if len(s) >= 2 and s[0 : 2] == '//':
            continue
        # ignore the space and tab
        real = ''
        for i in range(0, len(s)):
            if not IsSpace(s[i]):
                real += s[i]
        s = real
        del real
        ls: list[str] = s.split(',')
        for ele in ls:
            if ele == '': continue
            if ls.count('=') > 0:
                iden = ele.split('=')[0]
                ele = iden
                del iden
            output_file.write(f"case {ele}:\n\tbreak;\n")
        output_file.write('\n')
    output_file.close()
