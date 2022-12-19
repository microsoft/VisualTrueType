
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

const wchar_t  *gszVTTInstructionStr[256] =
       {
       /***** 0x00 - 0x0f *****/
       L"SVTCA[Y]",          
       L"SVTCA[X]",          
       L"SPVTCA[Y]",   
       L"SPVTCA[X]",   
       L"SFVTCA[Y]",   
       L"SFVTCA[X]",   
       L"SPVTL[r]",          
       L"SPVTL[R]",          
       L"SFVTL[r]",          
       L"SFVTL[R]",         
       L"SPVFS[]",                  
       L"SFVFS[]",                  
       L"GPV[]",                    
       L"GFV[]",                    
       L"SFVTPV[]",          
       L"ISECT[]",                  

       /***** 0x10 - 0x1f *****/
       L"SRP0[]",                   
       L"SRP1[]",                   
       L"SRP2[]",                   
       L"SZP0[]",                   
       L"SZP1[]",                   
       L"SZP2[]",                   
       L"SZPS[]",                   
       L"SLOOP[]",                  
       L"RTG[]",                    
       L"RTHG[]",                   
       L"SMD[]",                    
       L"ELSE[]",                   
	   L"JMPR[]",                   
       L"SCVTCI[]",          
       L"SSWCI[]",                  
       L"SSW[]",                    

       /***** 0x20 - 0x2f *****/
       L"DUP[]",                    
       L"POP[]",                    
       L"CLEAR[]",                  
       L"SWAP[]",                   
       L"DEPTH[]",                  
       L"CINDEX[]",          
       L"MINDEX[]",          
       L"ALIGNPTS[]",        
       L"RAW[]",       
       L"UTP[]",                    
       L"LOOPCALL[]",        
       L"CALL[]",                   
       L"FDEF[]",                   
       L"ENDF[]",                   
       L"MDAP[r]",
       L"MDAP[R]",


       /***** 0x30 - 0x3f *****/
       L"IUP[Y]",            
       L"IUP[X]",            
       L"SHP[2]",
       L"SHP[1]",
       L"SHC[2]",
       L"SHC[1]",
       L"SHZ[2]",
       L"SHZ[1]",
       L"SHPIX[]",
       L"IP[]",
       L"MSIRP[m]",
       L"MSIRP[M]",
       L"ALIGNRP[]",         
       L"RTDG[]",                   
       L"MIAP[r]",
       L"MIAP[R]",

       /***** 0x40 - 0x4f *****/
       L"NPUSHB[]",
       L"NPUSHW[]",
       L"WS[]",                     
       L"RS[]",                     
       L"WCVTP[]",                  
       L"RCVT[]",                   
       L"GC[N]",
       L"GC[O]",
       L"SCFS[]",                   
       L"MD[N]",
       L"MD[O]",
       L"MPPEM[]",                  
       L"MPS[]",                    
       L"FLIPON[]",          
       L"FLIPOFF[]",         
       L"DEBUG[]",                  

       /***** 0x50 - 0x5f *****/
       L"LT[]",                     
       L"LTEQ[]",                   
       L"GT[]",                     
       L"GTEQ[]",                   
       L"EQ[]",                     
       L"NEQ[]",                    
       L"ODD[]",                    
       L"EVEN[]",                   
       L"IF[]",                     
       L"EIF[]",                    
       L"AND[]",                    
       L"OR[]",                     
       L"NOT[]",                    
       L"DELTAP1[]",         
       L"SDB[]",                    
       L"SDS[]",                    

       /***** 0x60 - 0x6f *****/
       L"ADD[]",                    
       L"SUB[]",                    
       L"DIV[]",                    
       L"MUL[]",                    
       L"ABS[]",                    
       L"NEG[]",                    
       L"FLOOR[]",                  
       L"CEILING[]",         
       L"ROUND[Gr]",
       L"ROUND[Bl]",   
       L"ROUND[Wh]",   
       L"/* ROUND[3?] */",          
       L"NROUND[Gr]",  
       L"NROUND[Bl]",  
       L"NROUND[Wh]",  
       L"/* NROUND[3?] */",         

       /***** 0x70 - 0x7f *****/
       L"WCVTF[]",                  
       L"DELTAP2[]",         
       L"DELTAP3[]",         
       L"DELTAC1[]",         
       L"DELTAC2[]",         
       L"DELTAC3[]",         
       L"SROUND[]",          
       L"S45ROUND[]",        
       L"JROT[]",                   
	   L"JROF[]",                   
       L"ROFF[]",                   
       L"USER7B[]",
       L"RUTG[]",                   
       L"RDTG[]",                   
       L"SANGW[]",                  
       L"AA[]",                     

       /***** 0x80 - 0x8d *****/
       L"FLIPPT[]",          
       L"FLIPRGON[]",        
       L"FLIPRGOFF[]", 
       L"USER83[]",          
       L"USER84[]",          
       L"SCANCTRL[]",        
       L"SDPVTL[r]",   
       L"SDPVTL[R]",   
       L"GETINFO[]",         
       L"IDEF[]",                   
       L"ROLL[]",                   
       L"MAX[]",                    
       L"MIN[]",                    
       L"SCANTYPE[]",        
       L"INSTCTRL[]",        

       /***** 0x8f - 0xaf *****/
       L"USER8f[]",          
       L"USER90[]",          
       L"USER91[]",          
       L"USER92[]",          
       L"USER93[]",          
       L"USER94[]",          
       L"USER95[]",          
       L"USER96[]",          
       L"USER97[]",          
       L"USER98[]",          
       L"USER99[]",          
       L"USER9A[]",          
       L"USER9B[]",          
       L"USER9C[]",          
       L"USER9D[]",          
       L"USER9E[]",          
       L"USER9F[]",          
       L"USERA0[]",          
       L"USERA1[]",          
       L"USERA2[]",          
       L"USERA3[]",          
       L"USERA4[]",          
       L"USERA5[]",          
       L"USERA6[]",          
       L"USERA7[]",          
       L"USERA8[]",          
       L"USERA9[]",          
       L"USERAA[]",          
       L"USERAB[]",          
       L"USERAC[]",          
       L"USERAD[]",          
       L"USERAE[]",          
       L"USERAF[]",          
                     
       /***** 0xb0 - 0xb7 *****/
		L"PUSHB[1]",
		L"PUSHB[2]",
		L"PUSHB[3]",
		L"PUSHB[4]",
		L"PUSHB[5]",
		L"PUSHB[6]",
		L"PUSHB[7]",
		L"PUSHB[8]",
                     
       /***** 0xb8 - 0xbf *****/
		L"PUSHW[1]",
		L"PUSHW[2]",
		L"PUSHW[3]",
		L"PUSHW[4]",
		L"PUSHW[5]",
		L"PUSHW[6]",
		L"PUSHW[7]",
		L"PUSHW[8]",

       /***** 0xc0 - 0xdf *****/
       L"MDRP[m<rGr]",
       L"MDRP[m<rBl]",
       L"MDRP[m<rWh]",
       L"/* MDRP[m<r3] */",
       L"MDRP[m<RGr]",
       L"MDRP[m<RBl]",
       L"MDRP[m<RWh]",
       L"/* MDRP[m<R3] */",
       L"MDRP[m>rGr]",
       L"MDRP[m>rBl]",
       L"MDRP[m>rWh]",
       L"/* MDRP[m>r3] */",
       L"MDRP[m>RGr]",
       L"MDRP[m>RBl]",
       L"MDRP[m>RWh]",
       L"/* MDRP[m>R3] */",
       L"MDRP[M<rGr]",
       L"MDRP[M<rBl]",
       L"MDRP[M<rWh]",
       L"/* MDRP[M<r3] */",
       L"MDRP[M<RGr]",
       L"MDRP[M<RBl]",
       L"MDRP[M<RWh]",
       L"/* MDRP[M<R3] */",
       L"MDRP[M>rGr]",
       L"MDRP[M>rBl]",
       L"MDRP[M>rWh]",
       L"/* MDRP[M>r3] */",
       L"MDRP[M>RGr]",
       L"MDRP[M>RBl]",
       L"MDRP[M>RWh]",
       L"/* MDRP[M>R3] */",
                     
       /***** 0xe0 - 0xff *****/
       L"MIRP[m<rGr]",
       L"MIRP[m<rBl]",
       L"MIRP[m<rWh]",
       L"/* MIRP[m<r3] */",
       L"MIRP[m<RGr]",
       L"MIRP[m<RBl]",
       L"MIRP[m<RWh]",
       L"/* MIRP[m<R3] */",
       L"MIRP[m>rGr]",
       L"MIRP[m>rBl]",
       L"MIRP[m>rWh]",
       L"/* MIRP[m>r3] */",
       L"MIRP[m>RGr]",
       L"MIRP[m>RBl]",
       L"MIRP[m>RWh]",
       L"/* MIRP[m>R3] */",
       L"MIRP[M<rGr]",
       L"MIRP[M<rBl]",
       L"MIRP[M<rWh]",
       L"/* MIRP[M<r3] */",
       L"MIRP[M<RGr]",
       L"MIRP[M<RBl]",
       L"MIRP[M<RWh]",
       L"/* MIRP[M<R3] */",
       L"MIRP[M>rGr]",
       L"MIRP[M>rBl]",
       L"MIRP[M>rWh]",
       L"/* MIRP[M>r3] */",
       L"MIRP[M>RGr]",
       L"MIRP[M>RBl]",
       L"MIRP[M>RWh]",
       L"/* MIRP[M>R3] */" };       

/**********************************************************************/

void TTIUnAsm(unsigned char* pbyInst, unsigned short uIlen, TextBuffer* dest, bool clear, bool asmBlock)
{
	unsigned short	i;

	unsigned char	byInst;
	unsigned short	uloc;
	unsigned short	ucnt;
	short 	nval;
	unsigned char *pbyIP;	
	wchar_t szOutBuffer[256];

	if( clear )
	{
		dest->SetText(0,L""); 
	}

	if( uIlen == 0 )
	{
		// nada!       
		return;
	}

	if(asmBlock)		
		dest->Append(L"ASM(\"" BRK L"#PUSHOFF" BRK); 
	else
		dest->Append(L"#PUSHOFF" BRK); 


	pbyIP = pbyInst;
	uloc = 0;
	while ( pbyIP < pbyInst+uIlen )
		{
		byInst = *pbyIP++;

		/* format and print instruction */		
		swprintf(szOutBuffer,sizeof(szOutBuffer)/sizeof(wchar_t), L"%s", gszVTTInstructionStr[byInst] );

		dest->Append(szOutBuffer);		
		
		/* deal with any instruction arguments */
		if( byInst >= 0xb0  && byInst <= 0xb7 )			/*	PUSHB[abc] args */
			{ 			
			/* display PUSHB arguments */
			ucnt = (unsigned short) (byInst - 0xaf);
			for( i=0; i<ucnt; i++)
				{				
				nval = (short)*pbyIP++;
				swprintf(szOutBuffer,sizeof(szOutBuffer)/sizeof(wchar_t),L", %u", nval);
                dest->Append(szOutBuffer);
				uloc++;
				}			
			}
		else if( byInst >= 0xb8  &&  byInst <= 0xbf )	/*	PUSHW[abc] args */
			{ 			
			/* display PUSHW arguments */
			ucnt = (unsigned short) (byInst - 0xb7);
			for( i=0; i<ucnt; i++)
				{				
				nval = *pbyIP++;
				nval <<= 8;
				nval += (short) (*pbyIP++);    
				uloc += 2;
				swprintf(szOutBuffer,sizeof(szOutBuffer)/sizeof(wchar_t),L", %d", nval);
                dest->Append(szOutBuffer);
				}			
			}
		else if( byInst == 0x40 )								/* NPUSHB[] */
			{
			/* display NPUSHB arguments */
			ucnt = *pbyIP++;
			swprintf(szOutBuffer, sizeof(szOutBuffer) / sizeof(wchar_t), L", %u", ucnt);
			dest->Append(szOutBuffer);
			uloc++;			

			for( i=0; i<ucnt; i++)
				{				
				nval = (short)*pbyIP++;
				swprintf( szOutBuffer,sizeof(szOutBuffer)/sizeof(wchar_t),L", %u", nval);
                dest->Append(szOutBuffer);
				uloc++;
				}			
			}
		else if( byInst == 0x41 )				 				/* NPUSHW[] */	
			{
			/* display NPUSHW arguments */
			ucnt = *pbyIP++;
			swprintf(szOutBuffer, sizeof(szOutBuffer) / sizeof(wchar_t), L", %u", ucnt);
			dest->Append(szOutBuffer);
			uloc++;			

			for( i=0; i<ucnt; i++)
				{				
				nval = *pbyIP++;
				nval <<= 8;
				nval += (unsigned short) (*pbyIP++);  
				uloc += 2;
				swprintf(szOutBuffer,sizeof(szOutBuffer)/sizeof(wchar_t),L", %d", nval);
                dest->Append(szOutBuffer);
				}			
			}

        dest->Append(BRK);
		uloc ++;		/* update location */
		}

	if(asmBlock)		
		dest->Append(L"#PUSHON" BRK L"\")" BRK);
	else
		dest->Append(L"#PUSHON" BRK);

} /* TTIUnAsm() */
