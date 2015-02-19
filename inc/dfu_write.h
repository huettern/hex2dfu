

#ifndef DFU_WRITE_H
#define DFU_WRITE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*=============================================================================
MODULE DATA TYPES
=============================================================================*/
/// Models the basic DFU Image info
typedef struct {
    uint8_t bNumOfTargets;          ///!< Num of targets contained in the Img
    uint16_t wFWVersion;            ///!< Firmware version of the file
    uint16_t wVID;                  ///!< Vendor ID of the target uC
    uint16_t wPID;                  ///!< Product ID of the target uC
    char     sOutputFileName[255];  ///!< Output file name of DFU file
} DFU_tsImageInfo;

/// Models the Target Information
typedef struct {
    uint8_t  bAlternateSetting;     ///!< Gives the alternate setting
    char     szTargetName[255];      ///!< Target Name
    uint32_t lwTargetSize;          ///!< whole lenght of the associated image
    uint32_t lwNumOfElements;       ///!< How many elements the img is divided into
    uint32_t lwStartAdr;            ///!< Start address of the attached img
    uint8_t  *pbImage;              ///!< Pointer to the beginning of the first img
} DFU_tsTargetInfo;

/*=============================================================================
MODULE FUNCTION PROTOTYPES
=============================================================================*/
/// Fills the DFU Image Info with default values
/** Fills the DFU Image Info with default values
 *
 * @param filename   output filename
 */
DFU_tsImageInfo fNewImageInfo (const char* filename);

/// Fills the DFU Target Info with default values
/** Fills the DFU Target Info with default values
 *
 * @retval DFU_tsTargetInfo structure
 */
DFU_tsTargetInfo fNewTargetInfo (void);

/// Build Prefix and write it to output file
/** This method builds the DFU Prefix and writes it to the DFU output file
 *
 * @param pstImgInfo   Pointer to the DFU Image Info structure
 */
void fBuildPrefix (DFU_tsImageInfo *pstImgInfo);

/// Build Suffix and write it to outputfile
/** This method builds the DFU Suffix, calculated the CRC
 *  and writes it to the DFU output file
 *
 * @param pstImgInfo   Pointer to the DFU Image Info structure
 */
void fBuildSuffix (DFU_tsImageInfo *pstImgInfo);

/// Build Target Prefix, attach Target image and write it to output file
/** This method builds the DFU Target Prefix and writes a Image element
 *  including its image header and writes it to the DFU output file
 *
 * @param pstTgtInfo Pointer to a DFU Target Info structure
 */
void fBuildTarget (DFU_tsImageInfo *pstImgInfo, DFU_tsTargetInfo *pstTgtInfo);


#ifdef __cplusplus
} // extern "C"
#endif

#endif

