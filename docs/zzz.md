# ZZZ File

Files suffixed with the `.zzz` extension are compressed files using `zlib`.
The decompressed file size are the first 4 bytes, with the remainder of the
file being the compressed data.

#### Sample Decompression Program

An example program to decompress a .zzz file.

```cpp
#include "miniz.h"

void decompress_file(FILE* compressedFile)
{
	uint32_t decompressedSize, compressedSize;
	void *decompressedData, *compressedData;

	// get the total size of the compressed section
	fseek(compressedFile, 0, SEEK_END);
	compressedSize = ftell(compressedFile) - 4;

	// the first 4 bytes tell us how big the file is decompressed
	fseek(compressedFile, 0, SEEK_SET);
	fread(&decompressedSize, 4, 1, compressedFile);

	// allocate buffers for the data
	decompressedData = malloc(decompressedSize);
	compressedData = malloc(compressedSize);

	// read in the compressed data
	fread(compressedData, compressedSize, 1, compressedFile);
	
	int code = uncompress(decompressedData, &decompressedSize, compressedData, compressedSize);
	return code;
}
```