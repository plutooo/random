/*
  ARM CoreLink DMA-330 "Byte-code" Disassembler

  Note that the instructions are variable-length, so make sure the dump is
  aligned with the code you're trying to disassemble.

  - plutoo
*/

#include <stdio.h>
#include <stdlib.h>

void disasm(unsigned char* buf, long size) {
    #define left (size-p)

    long p = 0;
    unsigned char fb; // First byte.
    unsigned char sb; // Second byte.
    const char* suffix;
    unsigned int imm, reg;

    while(p < size) {
        fb = buf[p];

        printf("%04x | ", (unsigned int) p);

        if(fb == 0) {
            p++;
            printf("END                 ; Stop\n");
        }
        else if((fb & 0xFD) == 0x54) {
            if(left < 3) { printf("INVALID\n"); return; }
            
            p++;
            printf("ADDH %s, 0x%04x      ; Add 16-bit immediate\n",
                (fb & 2) ? "DAR" : "SAR", buf[p++] | (buf[p++] << 8));
        }
        else if((fb & 0xFD) == 0x5C) {
            if(left < 3) { printf("INVALID\n"); return; }
            
            p++;
            printf("ADNH %s, 0x%04x      ; Add negated 16-bit immediate\n",
                (fb & 2) ? "DAR" : "SAR", buf[p++] | (buf[p++] << 8));
        }
        else if(fb == 0x35) {
            if(left < 2) { printf("INVALID\n"); return; }

            p++;
            sb = buf[p++];

            if((sb & 7) == 0) {
                printf("FLUSHP 0x%02x         ; Flush peripheral\n", sb >> 3);
            }
            else { printf("INVALID\n"); return; }
        }
        else if((fb & 0xFD) == 0xA0) { // TODO: DMAGO
            if(left < 6) { printf("INVALID\n"); return; }

            p+=6;
            printf("GO\n");
        }
        else if(fb == 0x01) {
            p++;
            printf("KILL                ; Terminate thread\n");
        }
        else if((fb & 0xFC) == 0x04) {
            p++;

            if(fb & 1) {
                if(fb & 2) suffix = "B";
                else suffix = "S";
            }
            else {
                if(fb & 2) { printf("INVALID\n"); return; }
                else suffix = " ";
            }

            printf("LD%s                 ; Load\n", suffix);
        }
        else if(fb == 0xBC) {
            if(left < 6) { printf("INVALID\n"); return; }

            p++;
            sb = buf[p++];
            if(sb & 0xF8) { printf("INVALID\n"); return; }

            imm = buf[p++] | (buf[p++] << 8) | (buf[p++] << 16) |
                (buf[p++] << 24);
            reg = sb & 3;

            if(reg == 3) { printf("INVALID\n"); return; }

            printf("MOV %s, 0x%08x ; Move 32-bit immediate into register\n",
                (reg == 0) ? "SAR" : (reg == 1) ? "CCR" :
                (reg == 2) ? "DAR" : "", imm);
        }
        else if((fb & 0xFC) == 0x30) {
            if(left < 2) { printf("INVALID\n"); return; }

            p++;
            sb = buf[p++];

            if((sb & 7) == 0) {
                printf("WFP 0x%02x            ; Wait for peripheral\n",
                    sb >> 3);
            }
            else { printf("INVALID\n"); return; }
        }
        else if(fb == 0x13) {
            p++;
            printf("WMB                 ; Write memory barrier\n");
        }
        else if(fb == 0x12) {
            p++;
            printf("RMB                 ; Read memory barrier\n");
        }
        else if((fb & 0xFC) == 0x08) {
            p++;

            if(fb & 1) {
                if(fb & 2) suffix = "B";
                else suffix = "S";
            }
            else {
                if(fb & 2) { printf("INVALID\n"); return; }
                else suffix = " ";
            }

            printf("ST%s                 ; Store\n", suffix);
        }
        else if((fb & 0xFD) == 0x25) {
            if(left < 2) { printf("INVALID\n"); return; }

            if(fb & 2) suffix = "B";
            else suffix = "S";

            p++;
            sb = buf[p++];

            if((sb & 7) == 0) {
                printf("LDP%s 0x%02x           ; Load and notify peripheral\n",
                    suffix, sb >> 3);
            }
            else { printf("INVALID\n"); return; }
            
        }
        else if(fb == 0x34) {
            if(left < 2) { printf("INVALID\n"); return; }

            p++;
            sb = buf[p++];

            if((sb & 7) == 0) {
                printf("SEV 0x%02x            ; Send event\n", sb >> 3);
            }
            else { printf("INVALID\n"); return; }
            
        }
        else if((fb & 0xE8) == 0x28) {
            if(left < 2) { printf("INVALID\n"); return; }

            if(fb & 4) {
                if(fb & 0x10) { // TODO: DMALPEND S/B
                    p++;
                    sb = buf[p++];
                    printf("LPENDFE 0x%02x "
                        "; Loop-forever end (jump backwards)\n", sb);
                }
                else { // TODO: DMALPEND S/B
                    p++;
                    sb = buf[p++];
                    printf("LPEND.0 0x%02x "
                        "; Counter0-loop end (jump backwards)\n", sb);
                }
            }
            else {
                if(fb & 0x10) { // TODO: DMALPEND S/B
                    p++;
                    sb = buf[p++];
                    printf("LPEND.1 0x%02x "
                        "; Counter1-loop end (jump backwards)\n", sb);
                }
                else {
                    p++;
                    sb = buf[p++];

                    if(fb & 2) suffix = "B";
                    else suffix = "S";

                    if((sb & 7) == 0) {
                        printf("STP%s 0x%02x           ; "
                            "Store and notify peripheral\n", suffix, sb >> 3);
                    }
                    else { printf("INVALID\n"); return; }
                   
                }
            }
        }
        else if(fb == 0x18) {
            p++;
            printf("NOP ; No operation\n");
        }
        else if(fb == 0x0C) {
            p++;
            printf("STZ ; Store zero");
        }
        else if(fb == 0x36) {
            if(left < 2) { printf("INVALID\n"); return; }

            p++;
            sb = buf[p++];

            if(sb & 5) { printf("INVALID\n"); return; }

            if(sb & 2) {
                printf("INVALIDATE_ICACHE ; Invalidate instruction-cache\n");
            }
            else {
                printf("       ");
            }

            printf("WFE 0x%02x ; Wait for event\n", sb >> 3);
        }
        // LP
        else { printf("INVALID_\n"); return; }
    }
}

int main(int argc, char* argv[]) {
    FILE* fd;
    long size;
    int rc;
    char* buf;

    if(argc != 2) {
        printf("Usage: %s <dump.bin>\n", argv[0]);
        return 0;
    }

    fd = fopen(argv[1], "rb");
    if(fd == NULL) { perror("fopen"); return 1; }

    rc = fseek(fd, 0, SEEK_END);
    if(rc == -1) { perror("fseek"); goto err_file; }

    size = ftell(fd);
    if(size == -1) { perror("ftell"); goto err_file; }

    rc = fseek(fd, 0, SEEK_SET);
    if(rc == -1) { perror("fseek"); goto err_file; }

    buf = malloc(size);
    if(buf == NULL) { perror("malloc"); goto err_file; }
    
    rc = fread(buf, size, 1, fd);
    if(rc != 1) { perror("fread"); goto err_buf; }

    disasm(buf, size);
    
    fclose(fd);
    free(buf);
    return 0;

err_buf:
    free(buf);
err_file:
    fclose(fd);
    return 1;
}
