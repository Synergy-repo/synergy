#include <stdio.h>
#include <stdint.h>
#include <cpuid.h>
#include <x86intrin.h>

int
main ()
{
  unsigned int eax, ebx, ecx, edx;

  // Check if CPU supports XSAVE/XRSTOR by calling CPUID with EAX=0x1
  __cpuid ( 1, eax, ebx, ecx, edx );
  if ( !( ecx & bit_XSAVE ) )
    {
      printf ( "XSAVE not supported\n" );
      return 1;
    }

  // Get the size of the XSAVE area by calling CPUID with EAX=0xD and ECX=0
  __cpuid_count ( 0xD, 0, eax, ebx, ecx, edx );
  uint32_t xsave_area_size = ebx;

  printf ( "XSAVE area size: %u bytes\n", xsave_area_size );

  return 0;
}
