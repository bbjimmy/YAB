#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

unsigned long file_size(char *filename)
{
	unsigned long size;
    FILE *pFile = fopen(filename, "rb");
    fseek (pFile, 0, SEEK_END);
    size = ftell(pFile);
    fclose (pFile);
    return size;
}

int main(int argc, char *argv[])
{
	unsigned long filesize;
	char *buffer;
	FILE *fi, *fo;
    char *dest;
    unsigned long destlen;
    unsigned long i;
	
    if(argc != 2) {
        printf("Usage: yab-compress <file.yab>\n\n");
        return 1; 
    }

    filesize = file_size(argv[1]);
    buffer = (char*)malloc(filesize);
    dest = (char*)malloc(filesize);

    // read source file into buffer
    fi = fopen(argv[1], "r");
    fread(buffer, sizeof(char), filesize, fi);
    fclose(fi);

    // compress buffer
    compress(dest, &destlen, buffer, filesize);

    // write compressed buffer to output
    fo = fopen("program.h", "w");
    fprintf(fo, "const char myProg[] = {");
    for(i=0; i < destlen; i++)
        fprintf(fo, "%i,", dest[i]);
    fprintf(fo, "' ' };\n");
    fclose(fo);

	return 0;
}
