dec_str = ''
while True:
    dec_str = input()
    if dec_str == "Q" or dec_str == "q":
        break
    dec_str_arr = dec_str.strip().split(',')
    for el in dec_str_arr:
        print(f'{int(el):02x}', end=" ")
    print('')