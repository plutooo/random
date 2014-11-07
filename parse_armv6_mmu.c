/*
  parse_armv6_mmu.c - Parse ARMv6 L1/2 MMU tables dumped from 3DS.

  The input dump file should have the following format:
  0x0000-0x3000: L2 table
  0x3000-0xB000: L1 table

  TODO: When all L1 1MB descriptors are _identical_ there is what's called an ARM
        16MB supersection. They aren't detected atm.

  - plutoo
*/
#include <stdio.h>

typedef unsigned int u32;

#define BIT(x) (1<<(x))


const char* get_access(u32 APX, u32 AP) {
    if(APX == 0) {
        switch(AP) {
        case 0:
            return "Priv: --, User: --";
        case 1: 
            return "Priv: RW, User: --";
        case 2: 
            return "Priv: RW, User: R-";
        case 3: 
            return "Priv: RW, User: RW";
        }
    }
    else {
        switch(AP) {
        case 1: 
            return "Priv: R-, User: --";
        case 2: 
            return "Priv: R-, User: R-";
        case 3: 
            return "Priv: R-, User: R-";
        }
    }
    return "[UNKNOWN ACCESS]";
}

void parse_mmu(u32* L1, u32* L2, u32 L2_physaddr) {
    u32 upper;

    for(upper=0; upper<0x1000; upper++) {
        u32 virt_addr = upper << 20;
        u32 desc = L1[upper];
        u32 phys_addr;
        u32 size;
        u32 virt_addr_end;
        u32 phys_addr_end;
        u32 XN;
        u32 APX;
        u32 AP;

        u32 desc_type = desc & 3;
        switch(desc_type) {

        case 2: // L1: ARMv6+ 1MB section
            phys_addr = desc & 0xFFF00000;
            size = 0x100000;

            // Merge continous segments into one.
            while(upper < (0x1000-1)) {
                u32 next_desc = L1[upper+1];
                u32 next_phys_addr = next_desc & 0xFFF00000;

                if(next_phys_addr == (phys_addr + size) && 
                   (desc &~ 0xFFF00000) == (next_desc &~ 0xFFF00000))
                {
                    size += 0x100000;
                    upper++;
                }
                else break;
            }

            virt_addr_end = virt_addr + size;
            phys_addr_end = phys_addr + size;
            XN  = desc & BIT(4);
            APX = desc & BIT(15);
            AP = (desc >> 10) & 3;

            printf(" [L1 ] VA %08x..%08x -> PA %08x..%08x [ %s ] [ %s ]\n",
                   virt_addr, virt_addr_end,
                   phys_addr, phys_addr_end,
                   XN ? "XN" : " X",
                   get_access(APX, AP));
            break;

        case 1: // L2: Walking the second table.
            (void)0;

            u32 L2_addr = desc & 0xFFFFFFC0;
            if(L2_addr < L2_physaddr) {
                printf("Incorrect L2 address expected 0x%08x+.\n", L2_physaddr);
                break;
            }

            u32 middle;
            for(middle=0; middle<0x100; middle++) {
                virt_addr = (upper << 20) | (middle << 12);

                u32* L2_subtable = &L2[(L2_addr - L2_physaddr) / 4];
                u32  L2_desc = L2_subtable[middle];

                switch(L2_desc & 3) {

                case 3: // Extended page
                case 2:
                    phys_addr = L2_desc & 0xFFFFF000;
                    size = 0x1000;

                    // Merge continous segments into one.
                    while(middle < (0x100-1)) {
                        u32 next_L2_desc = L2_subtable[middle+1];
                        u32 next_phys_addr = next_L2_desc & 0xFFFFF000;

                        if(next_phys_addr == (phys_addr + size) && 
                           (L2_desc &~ 0xFFFFF000) == (next_L2_desc &~ 0xFFFFF000))
                        {
                            size += 0x1000;
                            middle++;
                        }
                        else break;
                    }

                    virt_addr_end = virt_addr + size;
                    phys_addr_end = phys_addr + size;
                    XN  = L2_desc & BIT(0);
                    APX = L2_desc & BIT(9);
                    AP = (L2_desc >> 4) & 3;

                    printf(" [L2S] VA %08x..%08x -> PA %08x..%08x [ %s ] [ %s ]\n",
                           virt_addr, virt_addr_end,
                           phys_addr, phys_addr_end,
                           XN ? "XN" : " X",
                           get_access(APX, AP));
                    break;

                case 1: // Large page
                    phys_addr = L2_desc & 0xFFFF0000;
                    size = 0x10000;

                    // Merge continous segments into one.
                    while(middle < (0x100-16)) {
                        u32 next_L2_desc = L2_subtable[middle+16];
                        u32 next_phys_addr = next_L2_desc & 0xFFFF0000;

                        if(next_phys_addr == (phys_addr + size) &&
                           (L2_desc &~ 0xFFFF0000) == (next_L2_desc &~ 0xFFFF0000))
                        {
                            size += 0x10000;
                            middle+=16;
                        }
                        else break;
                    }

                    virt_addr_end = virt_addr + size;
                    phys_addr_end = phys_addr + size;
                    XN  = L2_desc & BIT(15);
                    APX = L2_desc & BIT(9);
                    AP = (L2_desc >> 4) & 3;

                    printf(" [L2L] VA %08x..%08x -> PA %08x..%08x [ %s ] [ %s ]\n",
                           virt_addr, virt_addr_end,
                           phys_addr, phys_addr_end,
                           XN ? "XN" : " X",
                           get_access(APX, AP));

                    middle += 15;
                    break;

                default:
                    if(L2_desc != 0)
                        printf("Invalid L2 descriptor for vaddr %08x.\n", virt_addr);
                }

            }
            break;

        case 0:
            if(desc == 0)
                break;
            /* FALLTHROUGH */

        default:
            printf("Invalid L1 descriptor for vaddr %08x.\n", virt_addr);
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("usage: %s <memdump>\n", argv[0]);
        return 1;
    }

    FILE* fd = fopen(argv[1], "rb");
    if(fd == NULL) {
        perror("fopen");
        return 1;
    }

    u32 file[0xB000/4];
    fread(file, sizeof(file), 1, fd);

    u32* L1 = &file[0x3000/4];
    u32* L2 = &file[0/4];

    parse_mmu(L1, L2, 0x1FFF5000);

    fclose(fd);
    return 0;
}
