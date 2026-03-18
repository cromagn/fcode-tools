/**
 * FCode interface - Windows Fixed Version
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>  // Aggiunto per uint8_t e uint32_t

#include "ast.h"
#include "detokenize.h"
#include "fcode.h"
#include "pprint.h"
#include "words.h"

/* Fix per la compatibilità Windows: se O_BINARY non è definito (Unix), lo mettiamo a 0 */
#ifndef O_BINARY
#define O_BINARY 0
#endif

ssize_t fcode_read(uint8_t **fcodep, int fd) {
    uint8_t *fcode;
    uint32_t fcode_size, total_size, i = 0;
    ssize_t n;

    fcode = malloc(FCODE_HEADER_SIZE);
    if (!fcode) {
        perror("fcode_read()");
        return -1;
    }

    n = read(fd, fcode, FCODE_HEADER_SIZE);
    if (-1 == n) {
        perror("fcode_read()");
        return -1;
    } else if (n < FCODE_HEADER_SIZE) {
        fprintf(stderr, "fcode_read(): Short read\n");
        return -1;
    }

    if (fcode[i+0x00] == 0x55 && fcode[i+0x01] == 0xaa) {
        fcode = realloc(fcode, PCI_HEADER_SIZE+FCODE_HEADER_SIZE);
        if (!fcode) {
            perror("fcode_read()");
            return -1;
        }

        n = read(fd, &fcode[FCODE_HEADER_SIZE], PCI_HEADER_SIZE);
        if (-1 == n) {
            perror("fcode_read()");
            return -1;
        } else if (n < PCI_HEADER_SIZE) {
            fprintf(stderr, "fcode_read(): Short read\n");
            return -1;
        }

        i = PCI_HEADER_SIZE;
    }

    switch (fcode[i+0x00]) {
    case 0xf0:
    case 0xf1:
    case 0xf2:
    case 0xf3:
    case 0xfd:
        fcode_size =
            fcode[i+0x04]<<24|fcode[i+0x05]<<16|fcode[i+0x06]<<8|fcode[i+0x07];
        break;
    default:
        fprintf(stderr, "fcode_read(): No FCode header found\n");
        return -1;
    }

    total_size = i + fcode_size;

    fcode = realloc(fcode, total_size);
    if (!fcode) {
        perror("fcode_read()");
        return -1;
    }

    for (i = i + FCODE_HEADER_SIZE; i < total_size; i += n) {
        n = read(fd, &fcode[i], total_size - i);
        if (n < 0) {
            perror("fcode_read()");
            return -1;
        } else if (n == 0) {
            fprintf(stderr, "fcode_read(): Unexpected EOF\n");
            return -1;
        }
    }

    *fcodep = fcode;
    return total_size;
}

int fcode_detokenize(char *infname, char *outfname) {
    int infd;
    FILE *outfp = stdout;
    uint8_t *fcode;
    ast_t *T;

    /* AGGIUNTO O_BINARY: Evita che Windows si fermi al primo byte 0x1A */
    if ( (infd = open(infname, O_RDONLY | O_BINARY)) == -1 ) {
        perror("fcode_detokenize()");
        return 0;
    }

    if (outfname) {
        /* AGGIUNTO "wb": Evita che Windows aggiunga \r ai file di output */
        if ( !(outfp = fopen(outfname, "wb")) ) {
            perror("fcode_detokenize()");
            close(infd);
            return 0;
        }
    }

    if (fcode_read(&fcode, infd) == -1) {
        fprintf(stderr, "Failed to read FCode!\n");
        if (outfname) fclose(outfp);
        close(infd);
        return 0;
    } else if ( (T = detokenize(fcode)) == NULL ) {
        fprintf(stderr, "Failed to detokenize FCode!\n");
        if (outfname) fclose(outfp);
        close(infd);
        return 0;
    } else {
        prettyprint(outfp, T);
    }

    if (outfname)
        fclose(outfp);

    close(infd);
    return 1;
}

int fcode_copy(char *infname, char *outfname) {
    int infd, outfd;
    uint8_t *fcode;
    ssize_t total_size, n, i;

    /* AGGIUNTO O_BINARY */
    if ( (infd = open(infname, O_RDONLY | O_BINARY)) == -1 ) {
        perror("fcode_copy()");
        return 0;
    }

    if ( (total_size = fcode_read(&fcode, infd)) == -1 ) {
        fprintf(stderr, "fcode_copy(): Error reading FCode\n");
        close(infd);
        return 0;
    }

    /* AGGIUNTO O_BINARY */
    if ( (outfd = open(outfname, O_CREAT | O_WRONLY | O_BINARY, 0666)) == -1 ) {
        perror("fcode_copy()");
        close(infd);
        return 0;
    }

    for (i = 0; i < total_size; i += n) {
        n = write(outfd, &fcode[i], total_size - i);
        if (n < 0) {
            perror("fcode_copy()");
            close(outfd);
            close(infd);
            return 0;
        } else if (n == 0) {
            fprintf(stderr, "fcode_copy(): Unexpected EOF\n");
            close(outfd);
            close(infd);
            return 0;
        }
    }

    close(outfd);
    close(infd);
    return 1;
}
