/*
 * @file dfu_write.cpp
 *
 * @author Noah Huetter <noahhuetter@gmail.com>
 *
*/


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include "../inc/dfu_write.h"

using namespace std;

/*=============================================================================
PRIVATE DEFINES
=============================================================================*/
//DFU Prefix and Suffix
#define PFX_SIGNATURE "DfuSe"
#define PFX_SIGNATURE_SIZE 5
#define PFX_VERSION 0x01
#define DFU_PREFIX_SIZE 11

#define SFX_DEFAULT_VID 0x0483
#define SFX_DEFAULT_PID 0x0000
#define SFX_DEFAULT_FW  0x0000
#define SFX_CRC_SIZE 4
#define DFU_SUFFIX_SIZE 16

#define TARGET_PREFIX_SIGNATURE_SIZE	6
#define TARGET_PREFIX_SIGNATURE		"Target"

#define ELEMENT_HEADER_SIZE 8

//macro for CRC calculation
#define CRC(A,b) (A)=CrcTable[((A)^(b))&0xff]^((A)>>8)

/*=============================================================================
PRIVATE DATATYPES
=============================================================================*/
#pragma pack(push, 1) // exact fit - no padding
typedef struct
{
	char	    sSignature[PFX_SIGNATURE_SIZE];
	uint8_t	    bVersion;
	uint32_t	dwImageSize;
	uint8_t	    bTargets;
} tsDFUPrefix;
#pragma pack(pop) //back to whatever the previous packing mode was

typedef union
{
    tsDFUPrefix Prefix;
    uint8_t     bytewise[DFU_PREFIX_SIZE];
} tuPrefix;

#pragma pack(push, 1) // exact fit - no padding
typedef struct
{
	uint8_t bcdDeviceLo;
	uint8_t bcdDeviceHi;
	uint8_t idProductLo;
	uint8_t idProductHi;
	uint8_t idVendorLo;
	uint8_t idVendorHi;
	uint8_t bcdDFULo;
	uint8_t bcdDFUHi;
	char ucDfuSignature[3];
	uint8_t bLength;
	uint8_t dwCRC[4];
} tsDFUSuffix;
#pragma pack(pop) //back to whatever the previous packing mode was

typedef union
{
    tsDFUSuffix Suffix;
    uint8_t   bytewise[DFU_SUFFIX_SIZE];
} tuSuffix;

#pragma pack(push, 1) // exact fit - no padding
typedef struct
{
	char	sSignature[6];
	uint8_t	bAlternateSetting;
	uint32_t	bTargetNamed;
	char	szTargetName[255];
	uint32_t dwTargetSize;
	uint32_t dwNbElements;
} tsTargetPrefix;
#pragma pack(pop) //back to whatever the previous packing mode was

typedef union
{
    tsTargetPrefix Prefix;
    uint8_t   bytewise[274];
} tuTargetPrefix;

typedef union
{
    struct
    {
        uint32_t lwStartAdr;
        uint32_t lwSize;
    } Contents;
    uint8_t bytewise[8];
} tuElementHeader;



/*=============================================================================
PRIVATE MODULE FUNCTION PROTOTYPES
=============================================================================*/
/// Reads the current file size
/** Reads the file size of the given input stream
 *
 * @param ptr_ifile pointer to the input file stream
 * @return file size
 */
uint32_t fgetCurrentFilesize (istream *ptr_ifile);

/*=============================================================================
PRIVATE MODULE DATA
=============================================================================*/
const uint32_t CrcTable[] =	{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*=============================================================================
PRIVATE MODULE FUNCTIONS
=============================================================================*/
uint32_t fgetCurrentFilesize (istream *ptr_ifile)
{
    uint32_t filesize;
    uint32_t orig_position = ptr_ifile->tellg();

    //goto end of file
    ptr_ifile->clear();
    ptr_ifile->seekg(0, ios::end);
    filesize = ptr_ifile->tellg(); //get file size
    ptr_ifile->seekg(orig_position, ios::beg);

    return filesize;
}

/*=============================================================================
PUBLIC MODULE FUNCTIONS
=============================================================================*/
DFU_tsImageInfo fNewImageInfo (const char* filename)
{
	DFU_tsImageInfo tsInfo;

    tsInfo.bNumOfTargets = 1;
    tsInfo.wFWVersion = SFX_DEFAULT_FW;
    tsInfo.wPID = SFX_DEFAULT_PID;
    tsInfo.wVID = SFX_DEFAULT_VID;
    memset(tsInfo.sOutputFileName, 0, sizeof(tsInfo.sOutputFileName));
    memcpy(tsInfo.sOutputFileName, filename ,strlen(filename) );

    return tsInfo;
}


DFU_tsTargetInfo fNewTargetInfo (void)
{
    DFU_tsTargetInfo  tsTarget;

    tsTarget.bAlternateSetting = 0;
    tsTarget.lwNumOfElements = 0;
    tsTarget.lwTargetSize = 0;
    tsTarget.lwStartAdr = 0;
    tsTarget.pbImage = NULL;
    memset(tsTarget.szTargetName, 0, sizeof(tsTarget.szTargetName));

    return tsTarget;
}


void fBuildPrefix (DFU_tsImageInfo *pstImgInfo)
{
    ofstream file;
    tuPrefix pfx;

    //copy prefix information from ImgInfo to pfx union
    memcpy(pfx.Prefix.sSignature, PFX_SIGNATURE, PFX_SIGNATURE_SIZE);
    pfx.Prefix.bVersion = PFX_VERSION;
    pfx.Prefix.bTargets = pstImgInfo->bNumOfTargets;
    #warning TODO: get filesize
    pfx.Prefix.dwImageSize = 0x003cb1;
    //write file
    file.open(pstImgInfo->sOutputFileName, ios::out | ios::binary);
    file.write((const char*)&pfx.bytewise, sizeof(pfx.bytewise));
    file.flush();
    file.close();
}


void fBuildSuffix (DFU_tsImageInfo *pstImgInfo)
{
    ofstream file;
    ifstream fileread;
    tuSuffix sfx;
	uint32_t Size, i;
	uint32_t fullcrc = 0xffffffff;
	char *pBuffer;

    //copy data to data structure
    sfx.Suffix.bcdDeviceHi=(uint8_t)(pstImgInfo->wFWVersion>>8);
    sfx.Suffix.bcdDeviceLo=(uint8_t)pstImgInfo->wFWVersion;
    sfx.Suffix.idVendorHi=(uint8_t)(pstImgInfo->wVID>>8);
    sfx.Suffix.idVendorLo=(uint8_t)pstImgInfo->wVID;
    sfx.Suffix.idProductHi=(uint8_t)(pstImgInfo->wPID>>8);
    sfx.Suffix.idProductLo=(uint8_t)pstImgInfo->wPID;
    sfx.Suffix.bcdDFULo=0x1A;
    sfx.Suffix.bcdDFUHi=0x01;
    sfx.Suffix.ucDfuSignature[0]='U';
    sfx.Suffix.ucDfuSignature[1]='F';
    sfx.Suffix.ucDfuSignature[2]='D';
    sfx.Suffix.bLength=DFU_SUFFIX_SIZE;
    memset(sfx.Suffix.dwCRC, 0, sizeof(sfx.Suffix.dwCRC));

    //write Suffix
    file.open(pstImgInfo->sOutputFileName, ios::out | ios::binary | ios::app);
    file.write((const char*)&sfx.bytewise, ( sizeof(sfx.bytewise) - 4) );
    file.flush();
    file.close();

    //open file for reading to calculate CRC
    fileread.open(pstImgInfo->sOutputFileName, ios::out | ios::binary);

    //get filesize
    Size = fgetCurrentFilesize(&fileread); //get filesize without DRC suffix

    //read whole file without CRC into buffer
	pBuffer=new char[Size];
    fileread.clear();
    fileread.seekg(0, ios::beg);
	fileread.read(pBuffer, Size);//    m_File.Read(pBuffer, Size);
	//calc crc
    for (i=0;i<(Size);i++)
	{
		CRC(fullcrc, pBuffer[i]);
	}

    //write CRC to file
    file.open(pstImgInfo->sOutputFileName, ios::out | ios::binary | ios::app);
    file.seekp(Size, ios::beg); //goto CRC destination
    file.write((const char*)&fullcrc, sizeof(fullcrc));
    file.flush();

    //close files and clear buffer
    fileread.close();
    file.close();
	delete[] pBuffer;
}


void fBuildTarget (DFU_tsImageInfo *pstImgInfo, DFU_tsTargetInfo *pstTgtInfo)
{
    ofstream file;
    tuTargetPrefix  targetpfx;
    tuElementHeader header;

    //copy data of target header
    memcpy(targetpfx.Prefix.sSignature, TARGET_PREFIX_SIGNATURE, TARGET_PREFIX_SIGNATURE_SIZE);
    targetpfx.Prefix.bAlternateSetting = pstTgtInfo->bAlternateSetting;
    if(pstTgtInfo->szTargetName[0] == 0)
        targetpfx.Prefix.bTargetNamed = 0;
    else
        targetpfx.Prefix.bTargetNamed = 1;
    memcpy(targetpfx.Prefix.szTargetName, pstTgtInfo->szTargetName, sizeof(pstTgtInfo->szTargetName));
    targetpfx.Prefix.dwTargetSize = pstTgtInfo->lwTargetSize + ELEMENT_HEADER_SIZE;
    targetpfx.Prefix.dwNbElements = pstTgtInfo->lwNumOfElements;

    //copy data of element header
    header.Contents.lwStartAdr = pstTgtInfo->lwStartAdr;
    header.Contents.lwSize = pstTgtInfo->lwTargetSize;

    //write to file
    file.open(pstImgInfo->sOutputFileName, ios::out | ios::binary | ios::app);
    file.write((const char*)&targetpfx.bytewise, sizeof(targetpfx.bytewise));
    file.write((const char*)&header.bytewise, sizeof(header.bytewise));
    file.write((const char*)&pstTgtInfo->pbImage[pstTgtInfo->lwStartAdr], pstTgtInfo->lwTargetSize);
    file.flush();
    file.close();
}



