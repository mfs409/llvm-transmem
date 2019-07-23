import os
import re

def main():
    #read file
    file = open("memcached_f2.txt", "r")
    lines = file.readlines()
    file.close()
    outfile = open('memcached_fig2.txt', 'w')

    # look for patterns
    cntr = 0
    for line in lines:
        line = line.strip()
        pos = line.find("TPS")
        if pos != -1:
            cntr = cntr + 1
            word = re.findall(r'\d+',line[pos+5:pos+15])
            outfile.write(str(word[0]) + ',')
            if cntr == 15:
                cntr = 0;
                outfile.write('\n')

    os.remove("memcached_f2.txt")
    outfile.close()

main()

