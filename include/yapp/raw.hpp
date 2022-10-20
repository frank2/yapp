//! @file raw.hpp
//! @brief Raw headers, as seen in the `<windows.h>` include file.
//!
//! This header is written in this way to provide a cross-platform ability to access PE headers.
//! For Windows, defining the raw structures is redundant. Linux, however, needs definitions.
//! Shoving `<windows.h>` definitions into this header file makes dealing with cross-platform
//! PE needs a much less painful hassle.
//!

#pragma once

#include <yapp/platform.hpp>

#include <cstdint>
#include <cstddef>

#ifdef YAPP_WIN32
#include <windows.h>
#endif

namespace yapp
{
   /* first, define the constants */
   
#ifdef YAPP_WIN32
   /* to properly get the win32 header constants into our namespace, we need to undefine the preprocessor definitions and reinsert them as
      namespace consts */
   
#undef IMAGE_FILE_MACHINE_UNKNOWN    
#undef IMAGE_FILE_MACHINE_TARGET_HOST
#undef IMAGE_FILE_MACHINE_I386       
#undef IMAGE_FILE_MACHINE_R3000      
#undef IMAGE_FILE_MACHINE_R4000      
#undef IMAGE_FILE_MACHINE_R10000     
#undef IMAGE_FILE_MACHINE_WCEMIPSV2  
#undef IMAGE_FILE_MACHINE_ALPHA      
#undef IMAGE_FILE_MACHINE_SH3        
#undef IMAGE_FILE_MACHINE_SH3DSP     
#undef IMAGE_FILE_MACHINE_SH3E       
#undef IMAGE_FILE_MACHINE_SH4        
#undef IMAGE_FILE_MACHINE_SH5        
#undef IMAGE_FILE_MACHINE_ARM        
#undef IMAGE_FILE_MACHINE_THUMB      
#undef IMAGE_FILE_MACHINE_ARMNT      
#undef IMAGE_FILE_MACHINE_AM33       
#undef IMAGE_FILE_MACHINE_POWERPC    
#undef IMAGE_FILE_MACHINE_POWERPCFP  
#undef IMAGE_FILE_MACHINE_IA64       
#undef IMAGE_FILE_MACHINE_MIPS16     
#undef IMAGE_FILE_MACHINE_ALPHA64    
#undef IMAGE_FILE_MACHINE_MIPSFPU    
#undef IMAGE_FILE_MACHINE_MIPSFPU16  
#undef IMAGE_FILE_MACHINE_AXP64      
#undef IMAGE_FILE_MACHINE_TRICORE    
#undef IMAGE_FILE_MACHINE_CEF        
#undef IMAGE_FILE_MACHINE_EBC        
#undef IMAGE_FILE_MACHINE_AMD64      
#undef IMAGE_FILE_MACHINE_M32R       
#undef IMAGE_FILE_MACHINE_ARM64      
#undef IMAGE_FILE_MACHINE_CEE        

#undef IMAGE_FILE_RELOCS_STRIPPED    
#undef IMAGE_FILE_EXECUTABLE_IMAGE   
#undef IMAGE_FILE_LINE_NUMS_STRIPPED 
#undef IMAGE_FILE_LOCAL_SYMS_STRIPPED
#undef IMAGE_FILE_AGGRESIVE_WS_TRIM  
#undef IMAGE_FILE_LARGE_ADDRESS_AWARE
#undef IMAGE_FILE_BYTES_REVERSED_LO  
#undef IMAGE_FILE_32BIT_MACHINE      
#undef IMAGE_FILE_DEBUG_STRIPPED     
#undef IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP
#undef IMAGE_FILE_NET_RUN_FROM_SWAP      
#undef IMAGE_FILE_SYSTEM                 
#undef IMAGE_FILE_DLL                    
#undef IMAGE_FILE_UP_SYSTEM_ONLY         
#undef IMAGE_FILE_BYTES_REVERSED_HI

#undef IMAGE_NUMBEROF_DIRECTORY_ENTRIES

#undef IMAGE_SCN_TYPE_NO_PAD                
#undef IMAGE_SCN_CNT_CODE                   
#undef IMAGE_SCN_CNT_INITIALIZED_DATA       
#undef IMAGE_SCN_CNT_UNINITIALIZED_DATA     

#undef IMAGE_SCN_LNK_OTHER                  
#undef IMAGE_SCN_LNK_INFO                   
#undef IMAGE_SCN_LNK_REMOVE                 
#undef IMAGE_SCN_LNK_COMDAT                 
#undef IMAGE_SCN_NO_DEFER_SPEC_EXC          
#undef IMAGE_SCN_GPREL                      
#undef IMAGE_SCN_MEM_FARDATA                
#undef IMAGE_SCN_MEM_PURGEABLE              
#undef IMAGE_SCN_MEM_16BIT                  
#undef IMAGE_SCN_MEM_LOCKED                 
#undef IMAGE_SCN_MEM_PRELOAD                

#undef IMAGE_SCN_ALIGN_1BYTES               
#undef IMAGE_SCN_ALIGN_2BYTES               
#undef IMAGE_SCN_ALIGN_4BYTES               
#undef IMAGE_SCN_ALIGN_8BYTES               
#undef IMAGE_SCN_ALIGN_16BYTES              
#undef IMAGE_SCN_ALIGN_32BYTES              
#undef IMAGE_SCN_ALIGN_64BYTES              
#undef IMAGE_SCN_ALIGN_128BYTES             
#undef IMAGE_SCN_ALIGN_256BYTES             
#undef IMAGE_SCN_ALIGN_512BYTES             
#undef IMAGE_SCN_ALIGN_1024BYTES            
#undef IMAGE_SCN_ALIGN_2048BYTES            
#undef IMAGE_SCN_ALIGN_4096BYTES            
#undef IMAGE_SCN_ALIGN_8192BYTES            
#undef IMAGE_SCN_ALIGN_MASK                 

#undef IMAGE_SCN_LNK_NRELOC_OVFL            
#undef IMAGE_SCN_MEM_DISCARDABLE            
#undef IMAGE_SCN_MEM_NOT_CACHED             
#undef IMAGE_SCN_MEM_NOT_PAGED              
#undef IMAGE_SCN_MEM_SHARED                 
#undef IMAGE_SCN_MEM_EXECUTE                
#undef IMAGE_SCN_MEM_READ                   
#undef IMAGE_SCN_MEM_WRITE                  

#undef IMAGE_SIZEOF_SHORT_NAME

#undef IMAGE_DIRECTORY_ENTRY_EXPORT          
#undef IMAGE_DIRECTORY_ENTRY_IMPORT          
#undef IMAGE_DIRECTORY_ENTRY_RESOURCE        
#undef IMAGE_DIRECTORY_ENTRY_EXCEPTION       
#undef IMAGE_DIRECTORY_ENTRY_SECURITY        
#undef IMAGE_DIRECTORY_ENTRY_BASERELOC       
#undef IMAGE_DIRECTORY_ENTRY_DEBUG           
#undef IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    
#undef IMAGE_DIRECTORY_ENTRY_GLOBALPTR       
#undef IMAGE_DIRECTORY_ENTRY_TLS             
#undef IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    
#undef IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   
#undef IMAGE_DIRECTORY_ENTRY_IAT            
#undef IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   
#undef IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR

#undef IMAGE_REL_BASED_ABSOLUTE            
#undef IMAGE_REL_BASED_HIGH                
#undef IMAGE_REL_BASED_LOW                 
#undef IMAGE_REL_BASED_HIGHLOW             
#undef IMAGE_REL_BASED_HIGHADJ             
#undef IMAGE_REL_BASED_MACHINE_SPECIFIC_5  
#undef IMAGE_REL_BASED_RESERVED            
#undef IMAGE_REL_BASED_MACHINE_SPECIFIC_7  
#undef IMAGE_REL_BASED_MACHINE_SPECIFIC_8  
#undef IMAGE_REL_BASED_MACHINE_SPECIFIC_9  
#undef IMAGE_REL_BASED_DIR64

#undef IMAGE_DEBUG_TYPE_UNKNOWN                
#undef IMAGE_DEBUG_TYPE_COFF                   
#undef IMAGE_DEBUG_TYPE_CODEVIEW               
#undef IMAGE_DEBUG_TYPE_FPO                    
#undef IMAGE_DEBUG_TYPE_MISC                   
#undef IMAGE_DEBUG_TYPE_EXCEPTION              
#undef IMAGE_DEBUG_TYPE_FIXUP                  
#undef IMAGE_DEBUG_TYPE_OMAP_TO_SRC            
#undef IMAGE_DEBUG_TYPE_OMAP_FROM_SRC          
#undef IMAGE_DEBUG_TYPE_BORLAND                
#undef IMAGE_DEBUG_TYPE_RESERVED10             
#undef IMAGE_DEBUG_TYPE_CLSID                  
#undef IMAGE_DEBUG_TYPE_VC_FEATURE             
#undef IMAGE_DEBUG_TYPE_POGO                   
#undef IMAGE_DEBUG_TYPE_ILTCG                  
#undef IMAGE_DEBUG_TYPE_MPX                    
#undef IMAGE_DEBUG_TYPE_REPRO                  
#undef IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS  
#endif

   const std::uint16_t IMAGE_FILE_MACHINE_UNKNOWN =         0;
   const std::uint16_t IMAGE_FILE_MACHINE_TARGET_HOST =     0x0001;  // Useful for indicating we want to interact with the host and not a WoW guest.
   const std::uint16_t IMAGE_FILE_MACHINE_I386 =            0x014c;  // Intel 386.
   const std::uint16_t IMAGE_FILE_MACHINE_R3000 =           0x0162;  // MIPS little-endian, 0x160 big-endian
   const std::uint16_t IMAGE_FILE_MACHINE_R4000 =           0x0166;  // MIPS little-endian
   const std::uint16_t IMAGE_FILE_MACHINE_R10000 =          0x0168;  // MIPS little-endian
   const std::uint16_t IMAGE_FILE_MACHINE_WCEMIPSV2 =       0x0169;  // MIPS little-endian WCE v2
   const std::uint16_t IMAGE_FILE_MACHINE_ALPHA =           0x0184;  // Alpha_AXP
   const std::uint16_t IMAGE_FILE_MACHINE_SH3 =             0x01a2;  // SH3 little-endian
   const std::uint16_t IMAGE_FILE_MACHINE_SH3DSP =          0x01a3;
   const std::uint16_t IMAGE_FILE_MACHINE_SH3E =            0x01a4;  // SH3E little-endian
   const std::uint16_t IMAGE_FILE_MACHINE_SH4 =             0x01a6;  // SH4 little-endian
   const std::uint16_t IMAGE_FILE_MACHINE_SH5 =             0x01a8;  // SH5
   const std::uint16_t IMAGE_FILE_MACHINE_ARM =             0x01c0;  // ARM Little-Endian
   const std::uint16_t IMAGE_FILE_MACHINE_THUMB =           0x01c2;  // ARM Thumb/Thumb-2 Little-Endian
   const std::uint16_t IMAGE_FILE_MACHINE_ARMNT =           0x01c4;  // ARM Thumb-2 Little-Endian
   const std::uint16_t IMAGE_FILE_MACHINE_AM33 =            0x01d3;
   const std::uint16_t IMAGE_FILE_MACHINE_POWERPC =         0x01F0;  // IBM PowerPC Little-Endian
   const std::uint16_t IMAGE_FILE_MACHINE_POWERPCFP =       0x01f1;
   const std::uint16_t IMAGE_FILE_MACHINE_IA64 =            0x0200;  // Intel 64
   const std::uint16_t IMAGE_FILE_MACHINE_MIPS16 =          0x0266;  // MIPS
   const std::uint16_t IMAGE_FILE_MACHINE_ALPHA64 =         0x0284;  // ALPHA64
   const std::uint16_t IMAGE_FILE_MACHINE_MIPSFPU =         0x0366;  // MIPS
   const std::uint16_t IMAGE_FILE_MACHINE_MIPSFPU16 =       0x0466;  // MIPS
   const std::uint16_t IMAGE_FILE_MACHINE_AXP64 =           IMAGE_FILE_MACHINE_ALPHA64;
   const std::uint16_t IMAGE_FILE_MACHINE_TRICORE =         0x0520;  // Infineon
   const std::uint16_t IMAGE_FILE_MACHINE_CEF =             0x0CEF;
   const std::uint16_t IMAGE_FILE_MACHINE_EBC =             0x0EBC;  // EFI Byte Code
   const std::uint16_t IMAGE_FILE_MACHINE_AMD64 =           0x8664;  // AMD64 (K8)
   const std::uint16_t IMAGE_FILE_MACHINE_M32R =            0x9041;  // M32R little-endian
   const std::uint16_t IMAGE_FILE_MACHINE_ARM64 =           0xAA64;  // ARM64 Little-Endian
   const std::uint16_t IMAGE_FILE_MACHINE_CEE =             0xC0EE;

   const std::uint16_t IMAGE_FILE_RELOCS_STRIPPED =         0x0001;  // Relocation info stripped from file.
   const std::uint16_t IMAGE_FILE_EXECUTABLE_IMAGE =        0x0002;  // File is executable  (i.e. no unresolved external references).
   const std::uint16_t IMAGE_FILE_LINE_NUMS_STRIPPED =      0x0004;  // Line nunbers stripped from file.
   const std::uint16_t IMAGE_FILE_LOCAL_SYMS_STRIPPED =     0x0008;  // Local symbols stripped from file.
   const std::uint16_t IMAGE_FILE_AGGRESIVE_WS_TRIM =       0x0010;  // Aggressively trim working set
   const std::uint16_t IMAGE_FILE_LARGE_ADDRESS_AWARE =     0x0020;  // App can handle >2gb addresses
   const std::uint16_t IMAGE_FILE_BYTES_REVERSED_LO =       0x0080;  // Bytes of machine word are reversed.
   const std::uint16_t IMAGE_FILE_32BIT_MACHINE =           0x0100;  // 32 bit word machine.
   const std::uint16_t IMAGE_FILE_DEBUG_STRIPPED =          0x0200;  // Debugging info stripped from file in .DBG file
   const std::uint16_t IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400;  // If Image is on removable media, copy and run from the swap file.
   const std::uint16_t IMAGE_FILE_NET_RUN_FROM_SWAP =       0x0800;  // If Image is on Net, copy and run from the swap file.
   const std::uint16_t IMAGE_FILE_SYSTEM =                  0x1000;  // System File.
   const std::uint16_t IMAGE_FILE_DLL =                     0x2000;  // File is a DLL.
   const std::uint16_t IMAGE_FILE_UP_SYSTEM_ONLY =          0x4000;  // File should only be run on a UP machine
   const std::uint16_t IMAGE_FILE_BYTES_REVERSED_HI =       0x8000;  // Bytes of machine word are reversed.

   const std::uint32_t IMAGE_NUMBEROF_DIRECTORY_ENTRIES =   16;

   const std::uint32_t IMAGE_SCN_TYPE_NO_PAD =              0x00000008;  // Reserved.
   const std::uint32_t IMAGE_SCN_CNT_CODE =                 0x00000020;  // Section contains code.
   const std::uint32_t IMAGE_SCN_CNT_INITIALIZED_DATA =     0x00000040;  // Section contains initialized data.
   const std::uint32_t IMAGE_SCN_CNT_UNINITIALIZED_DATA =   0x00000080;  // Section contains uninitialized data.

   const std::uint32_t IMAGE_SCN_LNK_OTHER =                0x00000100;  // Reserved.
   const std::uint32_t IMAGE_SCN_LNK_INFO =                 0x00000200;  // Section contains comments or some other type of information.
   const std::uint32_t IMAGE_SCN_LNK_REMOVE =               0x00000800;  // Section contents will not become part of image.
   const std::uint32_t IMAGE_SCN_LNK_COMDAT =               0x00001000;  // Section contents comdat.
   const std::uint32_t IMAGE_SCN_NO_DEFER_SPEC_EXC =        0x00004000;  // Reset speculative exceptions handling bits in the TLB entries for this section.
   const std::uint32_t IMAGE_SCN_GPREL =                    0x00008000;  // Section content can be accessed relative to GP
   const std::uint32_t IMAGE_SCN_MEM_FARDATA =              0x00008000;
   const std::uint32_t IMAGE_SCN_MEM_PURGEABLE =            0x00020000;
   const std::uint32_t IMAGE_SCN_MEM_16BIT =                0x00020000;
   const std::uint32_t IMAGE_SCN_MEM_LOCKED =               0x00040000;
   const std::uint32_t IMAGE_SCN_MEM_PRELOAD =              0x00080000;

   const std::uint32_t IMAGE_SCN_ALIGN_1BYTES =             0x00100000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_2BYTES =             0x00200000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_4BYTES =             0x00300000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_8BYTES =             0x00400000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_16BYTES =            0x00500000;  // Default alignment if no others are specified.
   const std::uint32_t IMAGE_SCN_ALIGN_32BYTES =            0x00600000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_64BYTES =            0x00700000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_128BYTES =           0x00800000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_256BYTES =           0x00900000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_512BYTES =           0x00A00000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_1024BYTES =          0x00B00000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_2048BYTES =          0x00C00000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_4096BYTES =          0x00D00000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_8192BYTES =          0x00E00000;  //
   const std::uint32_t IMAGE_SCN_ALIGN_MASK =               0x00F00000;

   const std::uint32_t IMAGE_SCN_LNK_NRELOC_OVFL =          0x01000000;  // Section contains extended relocations.
   const std::uint32_t IMAGE_SCN_MEM_DISCARDABLE =          0x02000000;  // Section can be discarded.
   const std::uint32_t IMAGE_SCN_MEM_NOT_CACHED =           0x04000000;  // Section is not cachable.
   const std::uint32_t IMAGE_SCN_MEM_NOT_PAGED =            0x08000000;  // Section is not pageable.
   const std::uint32_t IMAGE_SCN_MEM_SHARED =               0x10000000;  // Section is shareable.
   const std::uint32_t IMAGE_SCN_MEM_EXECUTE =              0x20000000;  // Section is executable.
   const std::uint32_t IMAGE_SCN_MEM_READ =                 0x40000000;  // Section is readable.
   const std::uint32_t IMAGE_SCN_MEM_WRITE =                0x80000000;  // Section is writeable.

   const std::size_t IMAGE_SIZEOF_SHORT_NAME =              8;

   const std::size_t IMAGE_DIRECTORY_ENTRY_EXPORT =        0;   // Export Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_IMPORT =        1;   // Import Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_RESOURCE =      2;   // Resource Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_EXCEPTION =     3;   // Exception Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_SECURITY =      4;   // Security Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_BASERELOC =     5;   // Base Relocation Table
   const std::size_t IMAGE_DIRECTORY_ENTRY_DEBUG =         6;   // Debug Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_ARCHITECTURE =  7;   // Architecture Specific Data
   const std::size_t IMAGE_DIRECTORY_ENTRY_GLOBALPTR =     8;   // RVA of GP
   const std::size_t IMAGE_DIRECTORY_ENTRY_TLS =           9;   // TLS Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG =  10;   // Load Configuration Directory
   const std::size_t IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT = 11;   // Bound Import Directory in headers
   const std::size_t IMAGE_DIRECTORY_ENTRY_IAT =          12;   // Import Address Table
   const std::size_t IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT = 13;   // Delay Load Import Descriptors
   const std::size_t IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR=14;   // COM Runtime descriptor
   
   const std::uint8_t IMAGE_REL_BASED_ABSOLUTE =            0;
   const std::uint8_t IMAGE_REL_BASED_HIGH =                1;
   const std::uint8_t IMAGE_REL_BASED_LOW =                 2;
   const std::uint8_t IMAGE_REL_BASED_HIGHLOW =             3;
   const std::uint8_t IMAGE_REL_BASED_HIGHADJ =             4;
   const std::uint8_t IMAGE_REL_BASED_MACHINE_SPECIFIC_5 =  5;
   const std::uint8_t IMAGE_REL_BASED_RESERVED =            6;
   const std::uint8_t IMAGE_REL_BASED_MACHINE_SPECIFIC_7 =  7;
   const std::uint8_t IMAGE_REL_BASED_MACHINE_SPECIFIC_8 =  8;
   const std::uint8_t IMAGE_REL_BASED_MACHINE_SPECIFIC_9 =  9;
   const std::uint8_t IMAGE_REL_BASED_DIR64 =               10;

   const std::uint32_t IMAGE_DEBUG_TYPE_UNKNOWN =              0;
   const std::uint32_t IMAGE_DEBUG_TYPE_COFF =                 1;
   const std::uint32_t IMAGE_DEBUG_TYPE_CODEVIEW =             2;
   const std::uint32_t IMAGE_DEBUG_TYPE_FPO =                  3;
   const std::uint32_t IMAGE_DEBUG_TYPE_MISC =                 4;
   const std::uint32_t IMAGE_DEBUG_TYPE_EXCEPTION =            5;
   const std::uint32_t IMAGE_DEBUG_TYPE_FIXUP =                6;
   const std::uint32_t IMAGE_DEBUG_TYPE_OMAP_TO_SRC =          7;
   const std::uint32_t IMAGE_DEBUG_TYPE_OMAP_FROM_SRC =        8;
   const std::uint32_t IMAGE_DEBUG_TYPE_BORLAND =              9;
   const std::uint32_t IMAGE_DEBUG_TYPE_RESERVED10 =           10;
   const std::uint32_t IMAGE_DEBUG_TYPE_CLSID =                11;
   const std::uint32_t IMAGE_DEBUG_TYPE_VC_FEATURE =           12;
   const std::uint32_t IMAGE_DEBUG_TYPE_POGO =                 13;
   const std::uint32_t IMAGE_DEBUG_TYPE_ILTCG =                14;
   const std::uint32_t IMAGE_DEBUG_TYPE_MPX =                  15;
   const std::uint32_t IMAGE_DEBUG_TYPE_REPRO =                16;
   const std::uint32_t IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS =20;

   /* next, define the structures */

#ifdef YAPP_WIN32
   using IMAGE_DOS_HEADER = IMAGE_DOS_HEADER;
   using IMAGE_FILE_HEADER = IMAGE_FILE_HEADER;
   using IMAGE_DATA_DIRECTORY = IMAGE_DATA_DIRECTORY;
   using IMAGE_OPTIONAL_HEADER32 = IMAGE_OPTIONAL_HEADER32;
   using IMAGE_OPTIONAL_HEADER64 = IMAGE_OPTIONAL_HEADER64;
   using IMAGE_OPTIONAL_HEADER = IMAGE_OPTIONAL_HEADER;
   using IMAGE_NT_HEADERS32 = IMAGE_NT_HEADERS32;
   using IMAGE_NT_HEADERS64 = IMAGE_NT_HEADERS64;
   using IMAGE_NT_HEADERS = IMAGE_NT_HEADERS;
   using IMAGE_SECTION_HEADER = IMAGE_SECTION_HEADER;
   using IMAGE_EXPORT_DIRECTORY = IMAGE_EXPORT_DIRECTORY;
   using IMAGE_IMPORT_DESCRIPTOR = IMAGE_IMPORT_DESCRIPTOR;
   using IMAGE_IMPORT_BY_NAME = IMAGE_IMPORT_BY_NAME;
   using IMAGE_BASE_RELOCATION = IMAGE_BASE_RELOCATION;
   using IMAGE_RESOURCE_DIRECTORY = IMAGE_RESOURCE_DIRECTORY;
   using IMAGE_RESOURCE_DIRECTORY_ENTRY = IMAGE_RESOURCE_DIRECTORY_ENTRY;
   using IMAGE_RESOURCE_DIRECTORY_STRING = IMAGE_RESOURCE_DIRECTORY_STRING;
   using IMAGE_RESOURCE_DIR_STRING_U = IMAGE_RESOURCE_DIR_STRING_U;
   using IMAGE_RESOURCE_DATA_ENTRY = IMAGE_RESOURCE_DATA_ENTRY;
   using IMAGE_DEBUG_DIRECTORY = IMAGE_DEBUG_DIRECTORY;
   using IMAGE_TLS_DIRECTORY32 = IMAGE_TLS_DIRECTORY32;
   using IMAGE_TLS_DIRECTORY64 = IMAGE_TLS_DIRECTORY64;
   using IMAGE_TLS_DIRECTORY = IMAGE_TLS_DIRECTORY;
#else
   struct IMAGE_DOS_HEADER
   {
      std::uint16_t e_magic;
      std::uint16_t e_cblp;
      std::uint16_t e_cp;
      std::uint16_t e_crlc;
      std::uint16_t e_cparhdr;
      std::uint16_t e_minalloc;
      std::uint16_t e_maxalloc;
      std::uint16_t e_ss;
      std::uint16_t e_sp;
      std::uint16_t e_csum;
      std::uint16_t e_ip;
      std::uint16_t e_cs;
      std::uint16_t e_lfarlc;
      std::uint16_t e_ovno;
      std::uint16_t e_res[4];
      std::uint16_t e_oemid;
      std::uint16_t e_oeminfo;
      std::uint16_t e_res2[10];
      std::uint32_t e_lfanew;
   };

   struct IMAGE_FILE_HEADER {
      std::uint16_t Machine;
      std::uint16_t NumberOfSections;
      std::uint32_t TimeDateStamp;
      std::uint32_t PointerToSymbolTable;
      std::uint32_t NumberOfSymbols;
      std::uint16_t SizeOfOptionalHeader;
      std::uint16_t Characteristics;
   };

   struct IMAGE_DATA_DIRECTORY {
      std::uint32_t VirtualAddress;
      std::uint32_t Size;
   };

   struct IMAGE_OPTIONAL_HEADER32 {
      //
      // Standard fields.
      //

      std::uint8_t Magic;
      std::uint8_t MajorLinkerVersion;
      std::uint8_t MinorLinkerVersion;
      std::uint32_t SizeOfCode;
      std::uint32_t SizeOfInitializedData;
      std::uint32_t SizeOfUninitializedData;
      std::uint32_t AddressOfEntryPoint;
      std::uint32_t BaseOfCode;
      std::uint32_t BaseOfData;

      //
      // NT additional fields.
      //

      std::uint32_t ImageBase;
      std::uint32_t SectionAlignment;
      std::uint32_t FileAlignment;
      std::uint16_t MajorOperatingSystemVersion;
      std::uint16_t MinorOperatingSystemVersion;
      std::uint16_t MajorImageVersion;
      std::uint16_t MinorImageVersion;
      std::uint16_t MajorSubsystemVersion;
      std::uint16_t MinorSubsystemVersion;
      std::uint32_t Win32VersionValue;
      std::uint32_t SizeOfImage;
      std::uint32_t SizeOfHeaders;
      std::uint32_t CheckSum;
      std::uint16_t Subsystem;
      std::uint16_t DllCharacteristics;
      std::uint32_t SizeOfStackReserve;
      std::uint32_t SizeOfStackCommit;
      std::uint32_t SizeOfHeapReserve;
      std::uint32_t SizeOfHeapCommit;
      std::uint32_t LoaderFlags;
      std::uint32_t NumberOfRvaAndSizes;
      IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
   };

#ifdef YAPP_64BIT
   struct IMAGE_OPTIONAL_HEADER64 {
      std::uint16_t Magic;
      std::uint8_t MajorLinkerVersion;
      std::uint8_t MinorLinkerVersion;
      std::uint32_t SizeOfCode;
      std::uint32_t SizeOfInitializedData;
      std::uint32_t SizeOfUninitializedData;
      std::uint32_t AddressOfEntryPoint;
      std::uint32_t BaseOfCode;
      std::uint64_t ImageBase;
      std::uint32_t SectionAlignment;
      std::uint32_t FileAlignment;
      std::uint16_t MajorOperatingSystemVersion;
      std::uint16_t MinorOperatingSystemVersion;
      std::uint16_t MajorImageVersion;
      std::uint16_t MinorImageVersion;
      std::uint16_t MajorSubsystemVersion;
      std::uint16_t MinorSubsystemVersion;
      std::uint32_t Win32VersionValue;
      std::uint32_t SizeOfImage;
      std::uint32_t SizeOfHeaders;
      std::uint32_t CheckSum;
      std::uint16_t Subsystem;
      std::uint16_t DllCharacteristics;
      std::uint64_t SizeOfStackReserve;
      std::uint64_t SizeOfStackCommit;
      std::uint64_t SizeOfHeapReserve;
      std::uint64_t SizeOfHeapCommit;
      std::uint32_t LoaderFlags;
      std::uint32_t NumberOfRvaAndSizes;
      IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
   };
#endif

   #ifdef YAPP_64BIT
   using IMAGE_OPTIONAL_HEADER = IMAGE_OPTIONAL_HEADER64;
   #else
   using IMAGE_OPTIONAL_HEADER = IMAGE_OPTIONAL_HEADER32;
   #endif

#ifdef YAPP_64BIT
   struct IMAGE_NT_HEADERS64 {
      std::uint32_t Signature;
      IMAGE_FILE_HEADER FileHeader;
      IMAGE_OPTIONAL_HEADER64 OptionalHeader;
   };
#endif
   
   struct IMAGE_NT_HEADERS32 {
      std::uint32_t Signature;
      IMAGE_FILE_HEADER FileHeader;
      IMAGE_OPTIONAL_HEADER32 OptionalHeader;
   };

   #ifdef YAPP_64BIT
   using IMAGE_NT_HEADERS = IMAGE_NT_HEADERS64;
   #else
   using IMAGE_NT_HEADERS = IMAGE_NT_HEADERS32;
   #endif
   
   struct IMAGE_SECTION_HEADER {
      std::uint8_t    Name[IMAGE_SIZEOF_SHORT_NAME];
      union {
         std::uint32_t   PhysicalAddress;
         std::uint32_t   VirtualSize;
      } Misc;
      std::uint32_t   VirtualAddress;
      std::uint32_t   SizeOfRawData;
      std::uint32_t   PointerToRawData;
      std::uint32_t   PointerToRelocations;
      std::uint32_t   PointerToLinenumbers;
      std::uint16_t    NumberOfRelocations;
      std::uint16_t    NumberOfLinenumbers;
      std::uint32_t   Characteristics;
   };

   struct IMAGE_EXPORT_DIRECTORY {
      std::uint32_t Characteristics;
      std::uint32_t TimeDateStamp;
      std::uint16_t MajorVersion;
      std::uint16_t MinorVersion;
      std::uint32_t Name;
      std::uint32_t Base;
      std::uint32_t NumberOfFunctions;
      std::uint32_t NumberOfNames;
      std::uint32_t AddressOfFunctions;     // RVA from base of image
      std::uint32_t AddressOfNames;         // RVA from base of image
      std::uint32_t AddressOfNameOrdinals;  // RVA from base of image
   };

   struct IMAGE_IMPORT_DESCRIPTOR {
      union {
         std::uint32_t   Characteristics;            // 0 for terminating null import descriptor
         std::uint32_t   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
      } DUMMYUNIONNAME;
      std::uint32_t   TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

      std::uint32_t   ForwarderChain;                 // -1 if no forwarders
      std::uint32_t   Name;
      std::uint32_t   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
   };

   struct IMAGE_IMPORT_BY_NAME {
      std::uint16_t    Hint;
      char             Name[1];
   };

   struct IMAGE_BASE_RELOCATION {
      std::uint32_t   VirtualAddress;
      std::uint32_t   SizeOfBlock;
   };

   struct IMAGE_RESOURCE_DIRECTORY {
      std::uint32_t   Characteristics;
      std::uint32_t   TimeDateStamp;
      std::uint16_t   MajorVersion;
      std::uint16_t   MinorVersion;
      std::uint16_t   NumberOfNamedEntries;
      std::uint16_t   NumberOfIdEntries;
   };

   typedef struct IMAGE_RESOURCE_DIRECTORY_ENTRY {
      union {
         struct {
            std::uint32_t NameOffset:31;
            std::uint32_t NameIsString:1;
         } DUMMYSTRUCTNAME;
         std::uint32_t   Name;
         std::uint16_t    Id;
      } DUMMYUNIONNAME;
      union {
         std::uint32_t   OffsetToData;
         struct {
            std::uint32_t   OffsetToDirectory:31;
            std::uint32_t   DataIsDirectory:1;
         } DUMMYSTRUCTNAME2;
      } DUMMYUNIONNAME2;
   };

   struct IMAGE_RESOURCE_DIRECTORY_STRING {
      std::uint16_t    Length;
      char             NameString[ 1 ];
   };

   struct IMAGE_RESOURCE_DIR_STRING_U {
      std::uint16_t    Length;
      std::wchar_t     NameString[ 1 ];
   };

   struct IMAGE_RESOURCE_DATA_ENTRY {
      std::uint32_t   OffsetToData;
      std::uint32_t   Size;
      std::uint32_t   CodePage;
      std::uint32_t   Reserved;
   };

   struct IMAGE_DEBUG_DIRECTORY {
      std::uint32_t   Characteristics;
      std::uint32_t   TimeDateStamp;
      std::uint16_t   MajorVersion;
      std::uint16_t   MinorVersion;
      std::uint32_t   Type;
      std::uint32_t   SizeOfData;
      std::uint32_t   AddressOfRawData;
      std::uint32_t   PointerToRawData;
   };

   struct IMAGE_TLS_DIRECTORY32 {
      std::uint32_t   StartAddressOfRawData;
      std::uint32_t   EndAddressOfRawData;
      std::uint32_t   AddressOfIndex;             // Pstd::uint32_t
      std::uint32_t   AddressOfCallBacks;         // PIMAGE_TLS_CALLBACK *
      std::uint32_t   SizeOfZeroFill;
      union {
         std::uint32_t Characteristics;
         struct {
            std::uint32_t Reserved0 : 20;
            std::uint32_t Alignment : 4;
            std::uint32_t Reserved1 : 8;
         } DUMMYSTRUCTNAME;
      } DUMMYUNIONNAME;
   };

#ifdef YAPP_64BIT
   struct IMAGE_TLS_DIRECTORY64 {
      std::uint64_t StartAddressOfRawData;
      std::uint64_t EndAddressOfRawData;
      std::uint64_t AddressOfIndex;         // Pstd::uint32_t
      std::uint64_t AddressOfCallBacks;     // PIMAGE_TLS_CALLBACK *;
      std::uint32_t SizeOfZeroFill;
      union {
         std::uint32_t Characteristics;
         struct {
            std::uint32_t Reserved0 : 20;
            std::uint32_t Alignment : 4;
            std::uint32_t Reserved1 : 8;
         } DUMMYSTRUCTNAME;
      } DUMMYUNIONNAME;
   };
#endif

   #ifdef YAPP_64BIT
   using IMAGE_TLS_DIRECTORY = IMAGE_TLS_DIRECTORY64;
   #else
   using IMAGE_TLS_DIRECTORY = IMAGE_TLS_DIRECTORY32;
   #endif
#endif
}
