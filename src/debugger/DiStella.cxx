//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <queue>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bspf.hxx"
//#include "CartDebug.hxx"
#include "DiStella.hxx"

static void ADD_ENTRY(DisassemblyList& list, int address, const char* disasm, const char* bytes)
{
  DisassemblyTag t;
  t.address = address;
  t.disasm = disasm;
  t.bytes = bytes;
  list.push_back(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::DiStella()
  : mem(NULL),   /* copied data from the file-- can be from 2K-48K bytes in size */
    labels(NULL) /* array of information about addresses-- can be from 2K-48K bytes in size */
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::~DiStella()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DiStella::disassemble(DisassemblyList& list, const char* datafile)
{
  while(!myAddressQueue.empty())
    myAddressQueue.pop();

  app_data.start     = 0x0;
  app_data.load      = 0x0000;
  app_data.length    = 0;
  app_data.end       = 0x0FFF;
  app_data.disp_data = 0;

  /* Flag defaults */
  cflag = 0;
  dflag = 1;

  if (!file_load(datafile))
  {
    fprintf(stderr,"Unable to load %s\n", datafile);
    return -1;
  }
    
  /*====================================*/
  /* Allocate memory for "labels" variable */
  labels=(uInt8*) malloc(app_data.length);
  if (labels == NULL)
  {
    fprintf (stderr, "Malloc failed for 'labels' variable\n");
    return -1;
  }
  memset(labels,0,app_data.length);
  /*====================================*/


  /*-----------------------------------------------------
     The last 3 words of a program are as follows:

    .word INTERRUPT   (isr_adr)
    .word START       (start_adr)
    .word BRKroutine  (brk_adr)

     Since we always process START, move the Program
     Counter 3 bytes back from the final byte.
   -----------------------------------------------------*/

  pc=app_data.end-3;

  start_adr=read_adr();

  if (app_data.end == 0x7ff) /* 2K case */
  {
    /*============================================
       What is the offset?  Well, it's an address
       where the code segment starts.  For a 2K game,
       it is usually 0xf800, which would then have the
       code data end at 0xffff, but that is not
       necessarily the case.  Because the Atari 2600
       only has 13 address lines, it's possible that
       the "code" can be considered to start in a lot
       of different places.  So, we use the start
       address as a reference to determine where the
       offset is, logically-anded to produce an offset
       that is a multiple of 2K.

       Example:
         Start address = $D973, so therefore
         Offset to code = $D800
         Code range = $D800-$DFFF
     =============================================*/
    offset=(start_adr & 0xf800);
  }
  else if (app_data.end == 0xfff) /* 4K case */
  {
    /*============================================
       The offset is the address where the code segment
       starts.  For a 4K game, it is usually 0xf000,
       which would then have the code data end at 0xffff,
       but that is not necessarily the case.  Because the
       Atari 2600 only has 13 address lines, it's possible
       that the "code" can be considered to start in a lot
       of different places.  So, we use the start
       address as a reference to determine where the
       offset is, logically-anded to produce an offset
       that is a multiple of 4K.

       Example:
         Start address = $D973, so therefore
         Offset to code = $D000
         Code range = $D000-$DFFF
     =============================================*/
    offset=(start_adr - (start_adr % 0x1000));
  }

#if 0
  if (cflag && !load_config(config))
  {
    fprintf(stderr,"Unable to load config file %s\n",config);
    return -1;
  }
#endif

  myAddressQueue.push(start_adr);

  if (dflag)
  {
    while(!myAddressQueue.empty())
    {
      pc = myAddressQueue.front();
      pcbeg = pc;
      myAddressQueue.pop();
      disasm(pc, 1, list);
      for (int k = pcbeg; k <= pcend; k++)
        mark(k, REACHABLE);
    }
    
    for (int k = 0; k <= app_data.end; k++)
    {
      if (!check_bit(labels[k], REACHABLE))
        mark(k+offset, DATA);
    }
  }

  // Second pass
  disasm(offset, 2, list);

#if 0
    if (cflag) {
        printf("; %s contents:\n;\n",config);
        while (fgets(parms,79,cfg) != NULL)
            printf(";      %s",parms);
    }
  printf("\n");
#endif

  /* Print Line equates on screen */
#if 0
  for (int i = 0; i <= app_data.end; i++)
  {
    if ((labels[i] & (REFERENCED | VALID_ENTRY)) == REFERENCED)
    {
      /* so, if we have a piece of code referenced somewhere else, but cannot locate the label
         in code (i.e because the address is inside of a multi-byte instruction, then we
         print that address on screen for reference */
      printf("L%.4X   =   ",i+offset);
      printf("$%.4X\n",i+offset);
    }
  }
#endif

  // Third pass
  strcpy(linebuff,"");
  strcpy(nextline,"");
  disasm(offset, 3, list);

  free(labels);  /* Free dynamic memory before program ends */
  free(mem);     /* Free dynamic memory before program ends */

return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 DiStella::filesize(FILE *stream)
{
  uInt32 curpos, length;

  curpos = ftell(stream);
  fseek(stream, 0L, SEEK_END);
  length = ftell(stream);
  fseek(stream, curpos, SEEK_SET);
  return length;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 DiStella::read_adr()
{
  uInt8 d1,d2;

  d1 = mem[pc++];
  d2 = mem[pc++];

  return (uInt32) ((d2 << 8)+d1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DiStella::file_load(const char* file)
{
  FILE *fn;

  fn=fopen(file,"rb");
  if (fn == NULL)
    return 0;

  if (app_data.length == 0)
    app_data.length = filesize(fn);

  if (app_data.length == 2048)
    app_data.end = 0x7ff;
  else if (app_data.length == 4096)
    app_data.end = 0xfff;
  else
  {
    printf("Error: .bin file must be 2048 or 4096 bytes\n");
    exit(1);
  }

  /*====================================*/
  /* Dynamically allocate memory for "mem" variable */
  mem=(uInt8 *)malloc(app_data.length);
  if (mem == NULL)
  {
    printf ("Malloc failed for 'mem' variable\n");
    exit(1);
  }
  memset(mem,0,app_data.length);
  /*====================================*/

  rewind(fn); /* Point to beginning of file again */

  /* if no header exists, just read in the file data */
  fread(&mem[app_data.load],1,app_data.length,fn);

  fclose(fn); /* Data is read in, so close the file */

  if (app_data.start == 0)
    app_data.start = app_data.load;

  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DiStella::load_config(const char *file)
{
    char cfg_line[80];
    char cfg_tok[80];
    uInt32 cfg_beg, cfg_end;

    lineno=0;

    if ((cfg=fopen(file,"r")) == NULL)
        return 0;

    cfg_beg=cfg_end=0;

    while (fgets(cfg_line,79,cfg)!=NULL) {
        strcpy(cfg_tok,"");
        sscanf(cfg_line,"%s %x %x",cfg_tok,&cfg_beg,&cfg_end);
        if (!strcmp(cfg_tok,"DATA")) {
            check_range(cfg_beg,cfg_end);
            for(;cfg_beg<=cfg_end;) {
                mark(cfg_beg,DATA);
                if (cfg_beg == cfg_end)
                    cfg_end = 0;
                else
                    cfg_beg++;
            }
        } else if (!strcmp(cfg_tok,"GFX")) {
            check_range(cfg_beg,cfg_end);
            for(;cfg_beg<=cfg_end;) {
                mark(cfg_beg,GFX);
                if (cfg_beg == cfg_end)
                    cfg_end = 0;
                else
                    cfg_beg++;
            }
        } else if (!strcmp(cfg_tok,"ORG")) {
            offset = cfg_beg;
        } else if (!strcmp(cfg_tok,"CODE")) {
            check_range(cfg_beg,cfg_end);
            for(;cfg_beg<=cfg_end;) {
                mark(cfg_beg,REACHABLE);
                if (cfg_beg == cfg_end)
                    cfg_end = 0;
                else
                    cfg_beg++;
            }
        } else {
            fprintf(stderr,"Invalid line in config file - line %d ignored\n",lineno);
        }
    }
    rewind(cfg);
    return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::check_range(uInt32 beg, uInt32 end)
{
    lineno++;
    if (beg > end) {
        fprintf(stderr,"Beginning of range greater than End in config file in line %d\n",lineno);
        exit(1);
    }

    if (beg > app_data.end + offset) {
        fprintf(stderr,"Beginning of range out of range in line %d\n",lineno);
        exit(1);
    }

    if (beg < offset) {
        fprintf(stderr,"Beginning of range out of range in line %d\n",lineno);
        exit(1);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasm(uInt32 distart, int pass, DisassemblyList& list)
{
  uInt8 op, d1, opsrc;
  uInt32 ad;
  short amode;
  int i, bytes, labfound, addbranch;

  /* pc=app_data.start; */
  pc = distart - offset;
//cerr << " ==> pc = " << pc << "(" << pass << ")" << endl;
  while(pc <= app_data.end)
  {
#if 0
    if(pass == 3)
    {
      if (pc+offset == start_adr)
        printf("START:\n");
    }
#endif
    if(check_bit(labels[pc], GFX))
      /* && !check_bit(labels[pc], REACHABLE))*/
    {
      if (pass == 2)
        mark(pc+offset,VALID_ENTRY);
      else if (pass == 3)
      {
        if (check_bit(labels[pc],REFERENCED))
          printf("L%.4X: ",pc+offset);
        else
          printf("       ");

        printf(".byte $%.2X ; ",mem[pc]);
        showgfx(mem[pc]);
        printf(" $%.4X\n",pc+offset);
      }
      pc++;
    }
    else if (check_bit(labels[pc], DATA) && !check_bit(labels[pc], GFX))
        /* && !check_bit(labels[pc],REACHABLE)) {  */
    {
      mark(pc+offset, VALID_ENTRY);
      if (pass == 3)
      {
        bytes = 1;
        printf("L%.4X: .byte ",pc+offset);
        printf("$%.2X",mem[pc]);
      }
      pc++;

      while (check_bit(labels[pc], DATA) && !check_bit(labels[pc], REFERENCED)
             && !check_bit(labels[pc], GFX) && pass == 3 && pc <= app_data.end)
      {
        if (pass == 3)
        {
          bytes++;
          if (bytes == 17)
          {
            printf("\n       .byte $%.2X",mem[pc]);
            bytes = 1;
          }
          else
            printf(",$%.2X",mem[pc]);
        }
        pc++;
      }

      if (pass == 3)
        printf("\n");
    }
    else
    {
      op = mem[pc];
      /* version 2.1 bug fix */
      if (pass == 2)
        mark(pc+offset, VALID_ENTRY);
      else if (pass == 3)
      {
        if (check_bit(labels[pc], REFERENCED))
          printf("L%.4X: ", pc+offset);
        else
          printf("       ");
      }

      amode = ourLookup[op].addr_mode;
      if (app_data.disp_data)
      {
        for (i = 0; i < ourCLength[amode]; i++)
          if (pass == 3)
            printf("%02X ",mem[pc+i]);

        if (pass == 3)
          printf("  ");
      }

      pc++;

      if (ourLookup[op].mnemonic[0] == '.')
      {
        amode = IMPLIED;
        if (pass == 3)
        {
          sprintf(linebuff,".byte $%.2X ;",op);
          strcat(nextline,linebuff);
        }
      }

      if (pass == 1)
      {
        opsrc = ourLookup[op].source;
        /* M_REL covers BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ
           M_ADDR = JMP $NNNN, JSR $NNNN
           M_AIND = JMP Abs, Indirect */
        if ((opsrc == M_REL) || (opsrc == M_ADDR) || (opsrc == M_AIND))
          addbranch = 1;
        else
          addbranch = 0;
      }
      else if (pass == 3)
      {
        sprintf(linebuff,"%s",ourLookup[op].mnemonic);
        strcat(nextline,linebuff);
      }

      if (pc >= app_data.end)
      {
        switch(amode)
        {
          case ABSOLUTE:
          case ABSOLUTE_X:
          case ABSOLUTE_Y:
          case INDIRECT_X:
          case INDIRECT_Y:
          case ABS_INDIRECT:
          {
            if (pass == 3)
            {
              /* Line information is already printed; append .byte since last instruction will
                 put recompilable object larger that original binary file */
              printf(".byte $%.2X\n",op);

              if (pc == app_data.end)
              {
                if (check_bit(labels[pc],REFERENCED))
                  printf("L%.4X: ",pc+offset);
                else
                  printf("       ");

                op = mem[pc++];
                printf(".byte $%.2X\n",op);
              }
            }
            pcend = app_data.end + offset;
            return;
          }

          case ZERO_PAGE:
          case IMMEDIATE:
          case ZERO_PAGE_X:
          case ZERO_PAGE_Y:
          case RELATIVE:
          {
            if (pc > app_data.end)
            {
              if (pass == 3)
              {
                /* Line information is already printed, but we can remove the
                   Instruction (i.e. BMI) by simply clearing the buffer to print */
                strcpy(nextline,"");
                sprintf(linebuff,".byte $%.2X",op);
                strcat(nextline,linebuff);

                printf("%s",nextline);
                printf("\n");
                strcpy(nextline,"");
              }
              pc++;
              pcend = app_data.end + offset;
              return;
            }
          }

          default:
            break;
        }
      }

      /* Version 2.1 added the extensions to mnemonics */
      switch(amode)
      {
    #if 0
        case IMPLIED:
        {
          if (op == 0x40 || op == 0x60)
            if (pass == 3)
            {
              sprintf(linebuff,"\n");
              strcat(nextline,linebuff);
            }
          break;
        }
    #endif
        case ACCUMULATOR:
        {
          if (pass == 3)
            sprintf(linebuff,"    A");

          strcat(nextline,linebuff);
          break;
        }

        case ABSOLUTE:
        {
          ad = read_adr();
          labfound = mark(ad, REFERENCED);
          if (pass == 1)
          {
            if ((addbranch) && !check_bit(labels[ad & app_data.end], REACHABLE))
            {
              if (ad > 0xfff)
                myAddressQueue.push((ad & app_data.end) + offset);

              mark(ad, REACHABLE);
            }
          }
          else if (pass == 3)
          {
            if (ad < 0x100)
            {
              sprintf(linebuff,".w  ");
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    ");
              strcat(nextline,linebuff);
            }
            if (labfound == 1)
            {
              sprintf(linebuff,"L%.4X",ad);
              strcat(nextline,linebuff);
            }
            else if (labfound == 3)
            {
              sprintf(linebuff,"%s",ourIOMnemonic[ad-0x280]);
              strcat(nextline,linebuff);
            }
            else if (labfound == 4)
            {
              sprintf(linebuff,"L%.4X",(ad & app_data.end)+offset);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"$%.4X",ad);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case ZERO_PAGE:
        {
          d1 = mem[pc++];
          labfound = mark(d1, REFERENCED);
          if (pass == 3)
          {
            if (labfound == 2)
            {
              sprintf(linebuff,"    %s", ourTIAMnemonic[d1]);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    $%.2X ",d1);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case IMMEDIATE:
        {
          d1 = mem[pc++];
          if (pass == 3)
          {
            sprintf(linebuff,"    #$%.2X ",d1);
            strcat(nextline,linebuff);
          }
          break;
        }

        case ABSOLUTE_X:
        {
          ad = read_adr();
          labfound = mark(ad, REFERENCED);
          if (pass == 3)
          {
            if (ad < 0x100)
            {
              sprintf(linebuff,".wx ");
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    ");
              strcat(nextline,linebuff);
            }

            if (labfound == 1)
            {
              sprintf(linebuff,"L%.4X,X",ad);
              strcat(nextline,linebuff);
            }
            else if (labfound == 3)
            {
              sprintf(linebuff,"%s,X",ourIOMnemonic[ad-0x280]);
              strcat(nextline,linebuff);
            }
            else if (labfound == 4)
            {
              sprintf(linebuff,"L%.4X,X",(ad & app_data.end)+offset);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"$%.4X,X",ad);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case ABSOLUTE_Y:
        {
          ad = read_adr();
          labfound = mark(ad, REFERENCED);
          if (pass == 3)
          {
            if (ad < 0x100)
            {
              sprintf(linebuff,".wy ");
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    ");
              strcat(nextline,linebuff);
            }
            if (labfound == 1)
            {
              sprintf(linebuff,"L%.4X,Y",ad);
              strcat(nextline,linebuff);
            }
            else if (labfound == 3)
            {
              sprintf(linebuff,"%s,Y",ourIOMnemonic[ad-0x280]);
              strcat(nextline,linebuff);
            }
            else if (labfound == 4)
            {
              sprintf(linebuff,"L%.4X,Y",(ad & app_data.end)+offset);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"$%.4X,Y",ad);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case INDIRECT_X:
        {
          d1 = mem[pc++];
          if (pass == 3)
          {
            sprintf(linebuff,"    ($%.2X,X)",d1);
            strcat(nextline,linebuff);
          }
          break;
        }

        case INDIRECT_Y:
        {
          d1 = mem[pc++];
          if (pass == 3)
          {
            sprintf(linebuff,"    ($%.2X),Y",d1);
            strcat(nextline,linebuff);
          }
          break;
        }

        case ZERO_PAGE_X:
        {
          d1 = mem[pc++];
          labfound = mark(d1, REFERENCED);
          if (pass == 3)
          {
            if (labfound == 2)
            {
              sprintf(linebuff,"    %s,X", ourTIAMnemonic[d1]);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    $%.2X,X",d1);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case ZERO_PAGE_Y:
        {
          d1 = mem[pc++];
          labfound = mark(d1,REFERENCED);
          if (pass == 3)
          {
            if (labfound == 2)
            {
              sprintf(linebuff,"    %s,Y", ourTIAMnemonic[d1]);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    $%.2X,Y",d1);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case RELATIVE:
        {
          d1 = mem[pc++];
          ad = d1;
          if (d1 >= 128)
            ad = d1 - 256;

          labfound = mark(pc+ad+offset, REFERENCED);
          if (pass == 1)
          {
            if ((addbranch) && !check_bit(labels[pc+ad], REACHABLE))
            {
              myAddressQueue.push(pc+ad+offset);
              mark(pc+ad+offset, REACHABLE);
              /*       addressq=addq(addressq,pc+offset); */
            }
          }
          else if (pass == 3)
          {
            if (labfound == 1)
            {
              sprintf(linebuff,"    L%.4X",pc+ad+offset);
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    $%.4X",pc+ad+offset);
              strcat(nextline,linebuff);
            }
          }
          break;
        }

        case ABS_INDIRECT:
        {
          ad = read_adr();
          labfound = mark(ad, REFERENCED);
          if (pass == 3)
          {
            if (ad < 0x100)
            {
              sprintf(linebuff,".ind ");
              strcat(nextline,linebuff);
            }
            else
            {
              sprintf(linebuff,"    ");
              strcat(nextline,linebuff);
            }
          }
          if (labfound == 1)
          {
            sprintf(linebuff,"(L%04X)",ad);
            strcat(nextline,linebuff);
          }
          else if (labfound == 3)
          {
            sprintf(linebuff,"(%s)",ourIOMnemonic[ad-0x280]);
            strcat(nextline,linebuff);
          }
          else
          {
            sprintf(linebuff,"($%04X)",ad);
            strcat(nextline,linebuff);
          }
          break;
        }
      } // end switch

      if (pass == 1)
      {
        if (!strcmp(ourLookup[op].mnemonic,"RTS") ||
            !strcmp(ourLookup[op].mnemonic,"JMP") ||
            /* !strcmp(ourLookup[op].mnemonic,"BRK") || */
            !strcmp(ourLookup[op].mnemonic,"RTI"))
        {
          pcend = (pc-1) + offset;
          return;
        }
      }
      else if (pass == 3)
      {
        printf("%.4X |  %s", pc+offset, nextline);
//        printf("%s", nextline);
        if (strlen(nextline) <= 15)
        {
          /* Print spaces to align cycle count data */
          for (charcnt=0;charcnt<15-strlen(nextline);charcnt++)
            printf(" ");
        }
        printf(";%d",ourLookup[op].cycles);
        printf("\n");
        if (op == 0x40 || op == 0x60)
          printf("\n");

        strcpy(nextline,"");
      }
    }
  }  /* while loop */

  /* Just in case we are disassembling outside of the address range, force the pcend to EOF */
  pcend = app_data.end + offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DiStella::mark(uInt32 address, MarkType bit)
{
  /*-----------------------------------------------------------------------
    For any given offset and code range...

    If we're between the offset and the end of the code range, we mark
    the bit in the labels array for that data.  The labels array is an
    array of label info for each code address.  If this is the case,
    return "1", else...

    We sweep for hardware/system equates, which are valid addresses,
    outside the scope of the code/data range.  For these, we mark its
    corresponding hardware/system array element, and return "2" or "3"
    (depending on which system/hardware element was accessed).  If this
    was not the case...

    Next we check if it is a code "mirror".  For the 2600, address ranges
    are limited with 13 bits, so other addresses can exist outside of the
    standard code/data range.  For these, we mark the element in the "labels"
    array that corresponds to the mirrored address, and return "4"

    If all else fails, it's not a valid address, so return 0.

    A quick example breakdown for a 2600 4K cart:
    ===========================================================
      $00-$3d =     system equates (WSYNC, etc...); mark the array's element
                    with the appropriate bit; return 2.
      $0280-$0297 = system equates (INPT0, etc...); mark the array's element
                    with the appropriate bit; return 3.
      $1000-$1FFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $3000-$3FFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $5000-$5FFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $7000-$7FFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $9000-$9FFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $B000-$BFFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $D000-$DFFF = CODE/DATA, mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $F000-$FFFF = CODE/DATA, mark the code/data array for the address
                    with the appropriate bit; return 1.
      Anything else = invalid, return 0.
    ===========================================================
  -----------------------------------------------------------------------*/

  if (address >= offset && address <= app_data.end + offset)
  {
    labels[address-offset] = labels[address-offset] | bit;
    return 1;
  }
  else if (address >= 0 && address <= 0x3d)
  {
    reserved[address] = 1;
    return 2;
  }
  else if (address >= 0x280 && address <= 0x297)
  {
    ioresrvd[address-0x280] = 1;
    return 3;
  }
  else if (address > 0x1000)
  {
    /* 2K & 4K case */
    labels[address & app_data.end] = labels[address & app_data.end] | bit;
    return 4;
  }
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DiStella::check_bit(uInt8 bitflags, int i)
{
  return (int)(bitflags & i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::showgfx(uInt8 c)
{
  int i;

  printf("|");
  for(i = 0;i < 8; i++)
  {
    if (c > 127)
      printf("X");
    else
      printf(" ");

    c = c << 1;
  }
  printf("|");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DiStella::Instruction_tag DiStella::ourLookup[256] = {
/****  Positive  ****/

  /* 00 */ { "BRK",   IMPLIED,    M_NONE, M_PC,   7 }, /* Pseudo Absolute */
  /* 01 */ { "ORA",   INDIRECT_X, M_INDX, M_AC,   6 }, /* (Indirect,X) */
  /* 02 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 03 */ { ".SLO",  INDIRECT_X, M_INDX, M_INDX, 8 },

  /* 04 */ { ".NOOP", ZERO_PAGE,  M_NONE, M_NONE, 3 },
  /* 05 */ { "ORA",   ZERO_PAGE,  M_ZERO, M_AC,   3 }, /* Zeropage */
  /* 06 */ { "ASL",   ZERO_PAGE,  M_ZERO, M_ZERO, 5 }, /* Zeropage */
  /* 07 */ { ".SLO",  ZERO_PAGE,  M_ZERO, M_ZERO, 5 },

  /* 08 */ { "PHP",   IMPLIED,     M_SR,   M_NONE, 3 },
  /* 09 */ { "ORA",   IMMEDIATE,   M_IMM,  M_AC,   2 }, /* Immediate */
  /* 0a */ { "ASL",   ACCUMULATOR, M_AC,   M_AC,   2 }, /* Accumulator */
  /* 0b */ { ".ANC",  IMMEDIATE,   M_ACIM, M_ACNC, 2 },

  /* 0c */ { ".NOOP", ABSOLUTE, M_NONE, M_NONE, 4 },
  /* 0d */ { "ORA",   ABSOLUTE, M_ABS,  M_AC,   4 }, /* Absolute */
  /* 0e */ { "ASL",   ABSOLUTE, M_ABS,  M_ABS,  6 }, /* Absolute */
  /* 0f */ { ".SLO",  ABSOLUTE, M_ABS,  M_ABS,  6 },

  /* 10 */ { "BPL",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* 11 */ { "ORA",   INDIRECT_Y, M_INDY, M_AC,   5 }, /* (Indirect),Y */
  /* 12 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 13 */ { ".SLO",  INDIRECT_Y, M_INDY, M_INDY, 8 },

  /* 14 */ { ".NOOP", ZERO_PAGE_X, M_NONE, M_NONE, 4 },
  /* 15 */ { "ORA",   ZERO_PAGE_X, M_ZERX, M_AC,   4 }, /* Zeropage,X */
  /* 16 */ { "ASL",   ZERO_PAGE_X, M_ZERX, M_ZERX, 6 }, /* Zeropage,X */
  /* 17 */ { ".SLO",  ZERO_PAGE_X, M_ZERX, M_ZERX, 6 },

  /* 18 */ { "CLC",   IMPLIED,    M_NONE, M_FC,   2 },
  /* 19 */ { "ORA",   ABSOLUTE_Y, M_ABSY, M_AC,   4 }, /* Absolute,Y */
  /* 1a */ { ".NOOP", IMPLIED,    M_NONE, M_NONE, 2 },
  /* 1b */ { ".SLO",  ABSOLUTE_Y, M_ABSY, M_ABSY, 7 },

  /* 1c */ { ".NOOP", ABSOLUTE_X, M_NONE, M_NONE, 4 },
  /* 1d */ { "ORA",   ABSOLUTE_X, M_ABSX, M_AC,   4 }, /* Absolute,X */
  /* 1e */ { "ASL",   ABSOLUTE_X, M_ABSX, M_ABSX, 7 }, /* Absolute,X */
  /* 1f */ { ".SLO",  ABSOLUTE_X, M_ABSX, M_ABSX, 7 },

  /* 20 */ { "JSR",   ABSOLUTE,   M_ADDR, M_PC,   6 },
  /* 21 */ { "AND",   INDIRECT_X, M_INDX, M_AC,   6 }, /* (Indirect ,X) */
  /* 22 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 23 */ { ".RLA",  INDIRECT_X, M_INDX, M_INDX, 8 },

  /* 24 */ { "BIT",   ZERO_PAGE, M_ZERO, M_NONE, 3 }, /* Zeropage */
  /* 25 */ { "AND",   ZERO_PAGE, M_ZERO, M_AC,   3 }, /* Zeropage */
  /* 26 */ { "ROL",   ZERO_PAGE, M_ZERO, M_ZERO, 5 }, /* Zeropage */
  /* 27 */ { ".RLA",  ZERO_PAGE, M_ZERO, M_ZERO, 5 },

  /* 28 */ { "PLP",   IMPLIED,     M_NONE, M_SR,   4 },
  /* 29 */ { "AND",   IMMEDIATE,   M_IMM,  M_AC,   2 }, /* Immediate */
  /* 2a */ { "ROL",   ACCUMULATOR, M_AC,   M_AC,   2 }, /* Accumulator */
  /* 2b */ { ".ANC",  IMMEDIATE,   M_ACIM, M_ACNC, 2 },

  /* 2c */ { "BIT",   ABSOLUTE, M_ABS, M_NONE, 4 }, /* Absolute */
  /* 2d */ { "AND",   ABSOLUTE, M_ABS, M_AC,   4 }, /* Absolute */
  /* 2e */ { "ROL",   ABSOLUTE, M_ABS, M_ABS,  6 }, /* Absolute */
  /* 2f */ { ".RLA",  ABSOLUTE, M_ABS, M_ABS,  6 },

  /* 30 */ { "BMI",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* 31 */ { "AND",   INDIRECT_Y, M_INDY, M_AC,   5 }, /* (Indirect),Y */
  /* 32 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 33 */ { ".RLA",  INDIRECT_Y, M_INDY, M_INDY, 8 },

  /* 34 */ { ".NOOP", ZERO_PAGE_X, M_NONE, M_NONE, 4 },
  /* 35 */ { "AND",   ZERO_PAGE_X, M_ZERX, M_AC,   4 }, /* Zeropage,X */
  /* 36 */ { "ROL",   ZERO_PAGE_X, M_ZERX, M_ZERX, 6 }, /* Zeropage,X */
  /* 37 */ { ".RLA",  ZERO_PAGE_X, M_ZERX, M_ZERX, 6 },

  /* 38 */ { "SEC",   IMPLIED,    M_NONE, M_FC,   2 },
  /* 39 */ { "AND",   ABSOLUTE_Y, M_ABSY, M_AC,   4 }, /* Absolute,Y */
  /* 3a */ { ".NOOP", IMPLIED,    M_NONE, M_NONE, 2 },
  /* 3b */ { ".RLA",  ABSOLUTE_Y, M_ABSY, M_ABSY, 7 },

  /* 3c */ { ".NOOP", ABSOLUTE_X, M_NONE, M_NONE, 4 },
  /* 3d */ { "AND",   ABSOLUTE_X, M_ABSX, M_AC,   4 }, /* Absolute,X */
  /* 3e */ { "ROL",   ABSOLUTE_X, M_ABSX, M_ABSX, 7 }, /* Absolute,X */
  /* 3f */ { ".RLA",  ABSOLUTE_X, M_ABSX, M_ABSX, 7 },

  /* 40 */ { "RTI" ,  IMPLIED,    M_NONE, M_PC,   6 },
  /* 41 */ { "EOR",   INDIRECT_X, M_INDX, M_AC,   6 }, /* (Indirect,X) */
  /* 42 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 43 */ { ".SRE",  INDIRECT_X, M_INDX, M_INDX, 8 },

  /* 44 */ { ".NOOP", ZERO_PAGE, M_NONE, M_NONE, 3 },
  /* 45 */ { "EOR",   ZERO_PAGE, M_ZERO, M_AC,   3 }, /* Zeropage */
  /* 46 */ { "LSR",   ZERO_PAGE, M_ZERO, M_ZERO, 5 }, /* Zeropage */
  /* 47 */ { ".SRE",  ZERO_PAGE, M_ZERO, M_ZERO, 5 },

  /* 48 */ { "PHA",   IMPLIED,     M_AC,   M_NONE, 3 },
  /* 49 */ { "EOR",   IMMEDIATE,   M_IMM,  M_AC,   2 }, /* Immediate */
  /* 4a */ { "LSR",   ACCUMULATOR, M_AC,   M_AC,   2 }, /* Accumulator */
  /* 4b */ { ".ASR",  IMMEDIATE,   M_ACIM, M_AC,   2 }, /* (AC & IMM) >>1 */

  /* 4c */ { "JMP",   ABSOLUTE, M_ADDR, M_PC,  3 }, /* Absolute */
  /* 4d */ { "EOR",   ABSOLUTE, M_ABS,  M_AC,  4 }, /* Absolute */
  /* 4e */ { "LSR",   ABSOLUTE, M_ABS,  M_ABS, 6 }, /* Absolute */
  /* 4f */ { ".SRE",  ABSOLUTE, M_ABS,  M_ABS, 6 },

  /* 50 */ { "BVC",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* 51 */ { "EOR",   INDIRECT_Y, M_INDY, M_AC,   5 }, /* (Indirect),Y */
  /* 52 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 53 */ { ".SRE",  INDIRECT_Y, M_INDY, M_INDY, 8 },

  /* 54 */ { ".NOOP", ZERO_PAGE_X, M_NONE, M_NONE, 4 },
  /* 55 */ { "EOR",   ZERO_PAGE_X, M_ZERX, M_AC,   4 }, /* Zeropage,X */
  /* 56 */ { "LSR",   ZERO_PAGE_X, M_ZERX, M_ZERX, 6 }, /* Zeropage,X */
  /* 57 */ { ".SRE",  ZERO_PAGE_X, M_ZERX, M_ZERX, 6 },

  /* 58 */ { "CLI",   IMPLIED,    M_NONE, M_FI,   2 },
  /* 59 */ { "EOR",   ABSOLUTE_Y, M_ABSY, M_AC,   4 }, /* Absolute,Y */
  /* 5a */ { ".NOOP", IMPLIED,    M_NONE, M_NONE, 2 },
  /* 5b */ { ".SRE",  ABSOLUTE_Y, M_ABSY, M_ABSY, 7 },

  /* 5c */ { ".NOOP", ABSOLUTE_X, M_NONE, M_NONE, 4 },
  /* 5d */ { "EOR",   ABSOLUTE_X, M_ABSX, M_AC,   4 }, /* Absolute,X */
  /* 5e */ { "LSR",   ABSOLUTE_X, M_ABSX, M_ABSX, 7 }, /* Absolute,X */
  /* 5f */ { ".SRE",  ABSOLUTE_X, M_ABSX, M_ABSX, 7 },

  /* 60 */ { "RTS",   IMPLIED,    M_NONE, M_PC,   6 },
  /* 61 */ { "ADC",   INDIRECT_X, M_INDX, M_AC,   6 }, /* (Indirect,X) */
  /* 62 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* 63 */ { ".RRA",  INDIRECT_X, M_INDX, M_INDX, 8 },

  /* 64 */ { ".NOOP", ZERO_PAGE, M_NONE, M_NONE, 3 },
  /* 65 */ { "ADC",   ZERO_PAGE, M_ZERO, M_AC,   3 }, /* Zeropage */
  /* 66 */ { "ROR",   ZERO_PAGE, M_ZERO, M_ZERO, 5 }, /* Zeropage */
  /* 67 */ { ".RRA",  ZERO_PAGE, M_ZERO, M_ZERO, 5 },

  /* 68 */ { "PLA",   IMPLIED,     M_NONE, M_AC, 4 },
  /* 69 */ { "ADC",   IMMEDIATE,   M_IMM,  M_AC, 2 }, /* Immediate */
  /* 6a */ { "ROR",   ACCUMULATOR, M_AC,   M_AC, 2 }, /* Accumulator */
  /* 6b */ { ".ARR",  IMMEDIATE,   M_ACIM, M_AC, 2 }, /* ARR isn't typo */

  /* 6c */ { "JMP",   ABS_INDIRECT, M_AIND, M_PC,  5 }, /* Indirect */
  /* 6d */ { "ADC",   ABSOLUTE,     M_ABS,  M_AC,  4 }, /* Absolute */
  /* 6e */ { "ROR",   ABSOLUTE,     M_ABS,  M_ABS, 6 }, /* Absolute */
  /* 6f */ { ".RRA",  ABSOLUTE,     M_ABS,  M_ABS, 6 },

  /* 70 */ { "BVS",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* 71 */ { "ADC",   INDIRECT_Y, M_INDY, M_AC,   5 }, /* (Indirect),Y */
  /* 72 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT relative? */
  /* 73 */ { ".RRA",  INDIRECT_Y, M_INDY, M_INDY, 8 },

  /* 74 */ { ".NOOP", ZERO_PAGE_X, M_NONE, M_NONE, 4 },
  /* 75 */ { "ADC",   ZERO_PAGE_X, M_ZERX, M_AC,   4 }, /* Zeropage,X */
  /* 76 */ { "ROR",   ZERO_PAGE_X, M_ZERX, M_ZERX, 6 }, /* Zeropage,X */
  /* 77 */ { ".RRA",  ZERO_PAGE_X, M_ZERX, M_ZERX, 6 },

  /* 78 */ { "SEI",   IMPLIED,    M_NONE, M_FI,   2 },
  /* 79 */ { "ADC",   ABSOLUTE_Y, M_ABSY, M_AC,   4 }, /* Absolute,Y */
  /* 7a */ { ".NOOP", IMPLIED,    M_NONE, M_NONE, 2 },
  /* 7b */ { ".RRA",  ABSOLUTE_Y, M_ABSY, M_ABSY, 7 },

  /* 7c */ { ".NOOP", ABSOLUTE_X, M_NONE, M_NONE, 4 },
  /* 7d */ { "ADC",   ABSOLUTE_X, M_ABSX, M_AC,   4 },  /* Absolute,X */
  /* 7e */ { "ROR",   ABSOLUTE_X, M_ABSX, M_ABSX, 7 },  /* Absolute,X */
  /* 7f */ { ".RRA",  ABSOLUTE_X, M_ABSX, M_ABSX, 7 },

  /****  Negative  ****/

  /* 80 */ { ".NOOP", IMMEDIATE,  M_NONE, M_NONE, 2 },
  /* 81 */ { "STA",   INDIRECT_X, M_AC,   M_INDX, 6 }, /* (Indirect,X) */
  /* 82 */ { ".NOOP", IMMEDIATE,  M_NONE, M_NONE, 2 },
  /* 83 */ { ".SAX",  INDIRECT_X, M_ANXR, M_INDX, 6 },

  /* 84 */ { "STY",   ZERO_PAGE, M_YR,   M_ZERO, 3 }, /* Zeropage */
  /* 85 */ { "STA",   ZERO_PAGE, M_AC,   M_ZERO, 3 }, /* Zeropage */
  /* 86 */ { "STX",   ZERO_PAGE, M_XR,   M_ZERO, 3 }, /* Zeropage */
  /* 87 */ { ".SAX",  ZERO_PAGE, M_ANXR, M_ZERO, 3 },

  /* 88 */ { "DEY",   IMPLIED,   M_YR,   M_YR,   2 },
  /* 89 */ { ".NOOP", IMMEDIATE, M_NONE, M_NONE, 2 },
  /* 8a */ { "TXA",   IMPLIED,   M_XR,   M_AC,   2 },
  /****  ver abnormal: usually AC = AC | #$EE & XR & #$oper  ****/
  /* 8b */ { ".ANE",  IMMEDIATE, M_AXIM, M_AC,   2 },

  /* 8c */ { "STY",   ABSOLUTE, M_YR,   M_ABS, 4 }, /* Absolute */
  /* 8d */ { "STA",   ABSOLUTE, M_AC,   M_ABS, 4 }, /* Absolute */
  /* 8e */ { "STX",   ABSOLUTE, M_XR,   M_ABS, 4 }, /* Absolute */
  /* 8f */ { ".SAX",  ABSOLUTE, M_ANXR, M_ABS, 4 },

  /* 90 */ { "BCC",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* 91 */ { "STA",   INDIRECT_Y, M_AC,   M_INDY, 6 }, /* (Indirect),Y */
  /* 92 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT relative? */
  /* 93 */ { ".SHA",  INDIRECT_Y, M_ANXR, M_STH0, 6 },

  /* 94 */ { "STY",   ZERO_PAGE_X, M_YR,   M_ZERX, 4 }, /* Zeropage,X */
  /* 95 */ { "STA",   ZERO_PAGE_X, M_AC,   M_ZERX, 4 }, /* Zeropage,X */
  /* 96 */ { "STX",   ZERO_PAGE_Y, M_XR,   M_ZERY, 4 }, /* Zeropage,Y */
  /* 97 */ { ".SAX",  ZERO_PAGE_Y, M_ANXR, M_ZERY, 4 },

  /* 98 */ { "TYA",   IMPLIED,    M_YR,   M_AC,   2 },
  /* 99 */ { "STA",   ABSOLUTE_Y, M_AC,   M_ABSY, 5 }, /* Absolute,Y */
  /* 9a */ { "TXS",   IMPLIED,    M_XR,   M_SP,   2 },
  /*** This s very mysterious comm AND ... */
  /* 9b */ { ".SHS",  ABSOLUTE_Y, M_ANXR, M_STH3, 5 },

  /* 9c */ { ".SHY",  ABSOLUTE_X, M_YR,   M_STH2, 5 },
  /* 9d */ { "STA",   ABSOLUTE_X, M_AC,   M_ABSX, 5 }, /* Absolute,X */
  /* 9e */ { ".SHX",  ABSOLUTE_Y, M_XR  , M_STH1, 5 },
  /* 9f */ { ".SHA",  ABSOLUTE_Y, M_ANXR, M_STH1, 5 },

  /* a0 */ { "LDY",   IMMEDIATE,  M_IMM,  M_YR,   2 }, /* Immediate */
  /* a1 */ { "LDA",   INDIRECT_X, M_INDX, M_AC,   6 }, /* (indirect,X) */
  /* a2 */ { "LDX",   IMMEDIATE,  M_IMM,  M_XR,   2 }, /* Immediate */
  /* a3 */ { ".LAX",  INDIRECT_X, M_INDX, M_ACXR, 6 }, /* (indirect,X) */

  /* a4 */ { "LDY",   ZERO_PAGE, M_ZERO, M_YR,   3 }, /* Zeropage */
  /* a5 */ { "LDA",   ZERO_PAGE, M_ZERO, M_AC,   3 }, /* Zeropage */
  /* a6 */ { "LDX",   ZERO_PAGE, M_ZERO, M_XR,   3 }, /* Zeropage */
  /* a7 */ { ".LAX",  ZERO_PAGE, M_ZERO, M_ACXR, 3 },

  /* a8 */ { "TAY",   IMPLIED,   M_AC,   M_YR,   2 },
  /* a9 */ { "LDA",   IMMEDIATE, M_IMM,  M_AC,   2 }, /* Immediate */
  /* aa */ { "TAX",   IMPLIED,   M_AC,   M_XR,   2 },
  /* ab */ { ".LXA",  IMMEDIATE, M_ACIM, M_ACXR, 2 }, /* LXA isn't a typo */

  /* ac */ { "LDY",   ABSOLUTE, M_ABS, M_YR,   4 }, /* Absolute */
  /* ad */ { "LDA",   ABSOLUTE, M_ABS, M_AC,   4 }, /* Absolute */
  /* ae */ { "LDX",   ABSOLUTE, M_ABS, M_XR,   4 }, /* Absolute */
  /* af */ { ".LAX",  ABSOLUTE, M_ABS, M_ACXR, 4 },

  /* b0 */ { "BCS",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* b1 */ { "LDA",   INDIRECT_Y, M_INDY, M_AC,   5 }, /* (indirect),Y */
  /* b2 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* b3 */ { ".LAX",  INDIRECT_Y, M_INDY, M_ACXR, 5 },

  /* b4 */ { "LDY",   ZERO_PAGE_X, M_ZERX, M_YR,   4 }, /* Zeropage,X */
  /* b5 */ { "LDA",   ZERO_PAGE_X, M_ZERX, M_AC,   4 }, /* Zeropage,X */
  /* b6 */ { "LDX",   ZERO_PAGE_Y, M_ZERY, M_XR,   4 }, /* Zeropage,Y */
  /* b7 */ { ".LAX",  ZERO_PAGE_Y, M_ZERY, M_ACXR, 4 },

  /* b8 */ { "CLV",   IMPLIED,    M_NONE, M_FV,   2 },
  /* b9 */ { "LDA",   ABSOLUTE_Y, M_ABSY, M_AC,   4 }, /* Absolute,Y */
  /* ba */ { "TSX",   IMPLIED,    M_SP,   M_XR,   2 },
  /* bb */ { ".LAS",  ABSOLUTE_Y, M_SABY, M_ACXS, 4 },

  /* bc */ { "LDY",   ABSOLUTE_X, M_ABSX, M_YR,   4 }, /* Absolute,X */
  /* bd */ { "LDA",   ABSOLUTE_X, M_ABSX, M_AC,   4 }, /* Absolute,X */
  /* be */ { "LDX",   ABSOLUTE_Y, M_ABSY, M_XR,   4 }, /* Absolute,Y */
  /* bf */ { ".LAX",  ABSOLUTE_Y, M_ABSY, M_ACXR, 4 },

  /* c0 */ { "CPY",   IMMEDIATE,  M_IMM,  M_NONE, 2 }, /* Immediate */
  /* c1 */ { "CMP",   INDIRECT_X, M_INDX, M_NONE, 6 }, /* (Indirect,X) */
  /* c2 */ { ".NOOP", IMMEDIATE,  M_NONE, M_NONE, 2 }, /* occasional TILT */
  /* c3 */ { ".DCP",  INDIRECT_X, M_INDX, M_INDX, 8 },

  /* c4 */ { "CPY",   ZERO_PAGE, M_ZERO, M_NONE, 3 }, /* Zeropage */
  /* c5 */ { "CMP",   ZERO_PAGE, M_ZERO, M_NONE, 3 }, /* Zeropage */
  /* c6 */ { "DEC",   ZERO_PAGE, M_ZERO, M_ZERO, 5 }, /* Zeropage */
  /* c7 */ { ".DCP",  ZERO_PAGE, M_ZERO, M_ZERO, 5 },

  /* c8 */ { "INY",   IMPLIED,   M_YR,  M_YR,   2 },
  /* c9 */ { "CMP",   IMMEDIATE, M_IMM, M_NONE, 2 }, /* Immediate */
  /* ca */ { "DEX",   IMPLIED,   M_XR,  M_XR,   2 },
  /* cb */ { ".SBX",  IMMEDIATE, M_IMM, M_XR,   2 },

  /* cc */ { "CPY",   ABSOLUTE, M_ABS, M_NONE, 4 }, /* Absolute */
  /* cd */ { "CMP",   ABSOLUTE, M_ABS, M_NONE, 4 }, /* Absolute */
  /* ce */ { "DEC",   ABSOLUTE, M_ABS, M_ABS,  6 }, /* Absolute */
  /* cf */ { ".DCP",  ABSOLUTE, M_ABS, M_ABS,  6 },

  /* d0 */ { "BNE",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* d1 */ { "CMP",   INDIRECT_Y, M_INDY, M_NONE, 5 }, /* (Indirect),Y */
  /* d2 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* d3 */ { ".DCP",  INDIRECT_Y, M_INDY, M_INDY, 8 },

  /* d4 */ { ".NOOP", ZERO_PAGE_X, M_NONE, M_NONE, 4 },
  /* d5 */ { "CMP",   ZERO_PAGE_X, M_ZERX, M_NONE, 4 }, /* Zeropage,X */
  /* d6 */ { "DEC",   ZERO_PAGE_X, M_ZERX, M_ZERX, 6 }, /* Zeropage,X */
  /* d7 */ { ".DCP",  ZERO_PAGE_X, M_ZERX, M_ZERX, 6 },

  /* d8 */ { "CLD",   IMPLIED,    M_NONE, M_FD,   2 },
  /* d9 */ { "CMP",   ABSOLUTE_Y, M_ABSY, M_NONE, 4 }, /* Absolute,Y */
  /* da */ { ".NOOP", IMPLIED,    M_NONE, M_NONE, 2 },
  /* db */ { ".DCP",  ABSOLUTE_Y, M_ABSY, M_ABSY, 7 },

  /* dc */ { ".NOOP", ABSOLUTE_X, M_NONE, M_NONE, 4 },
  /* dd */ { "CMP",   ABSOLUTE_X, M_ABSX, M_NONE, 4 }, /* Absolute,X */
  /* de */ { "DEC",   ABSOLUTE_X, M_ABSX, M_ABSX, 7 }, /* Absolute,X */
  /* df */ { ".DCP",  ABSOLUTE_X, M_ABSX, M_ABSX, 7 },

  /* e0 */ { "CPX",   IMMEDIATE,  M_IMM,  M_NONE, 2 }, /* Immediate */
  /* e1 */ { "SBC",   INDIRECT_X, M_INDX, M_AC,   6 }, /* (Indirect,X) */
  /* e2 */ { ".NOOP", IMMEDIATE,  M_NONE, M_NONE, 2 },
  /* e3 */ { ".ISB",  INDIRECT_X, M_INDX, M_INDX, 8 },

  /* e4 */ { "CPX",   ZERO_PAGE, M_ZERO, M_NONE, 3 }, /* Zeropage */
  /* e5 */ { "SBC",   ZERO_PAGE, M_ZERO, M_AC,   3 }, /* Zeropage */
  /* e6 */ { "INC",   ZERO_PAGE, M_ZERO, M_ZERO, 5 }, /* Zeropage */
  /* e7 */ { ".ISB",  ZERO_PAGE, M_ZERO, M_ZERO, 5 },

  /* e8 */ { "INX",   IMPLIED,   M_XR,   M_XR,   2 },
  /* e9 */ { "SBC",   IMMEDIATE, M_IMM,  M_AC,   2 }, /* Immediate */
  /* ea */ { "NOP",   IMPLIED,   M_NONE, M_NONE, 2 },
  /* eb */ { ".USBC", IMMEDIATE, M_IMM,  M_AC,   2 }, /* same as e9 */

  /* ec */ { "CPX",   ABSOLUTE, M_ABS, M_NONE, 4 }, /* Absolute */
  /* ed */ { "SBC",   ABSOLUTE, M_ABS, M_AC,   4 }, /* Absolute */
  /* ee */ { "INC",   ABSOLUTE, M_ABS, M_ABS,  6 }, /* Absolute */
  /* ef */ { ".ISB",  ABSOLUTE, M_ABS, M_ABS,  6 },

  /* f0 */ { "BEQ",   RELATIVE,   M_REL,  M_NONE, 2 },
  /* f1 */ { "SBC",   INDIRECT_Y, M_INDY, M_AC,   5 }, /* (Indirect),Y */
  /* f2 */ { ".JAM",  IMPLIED,    M_NONE, M_NONE, 0 }, /* TILT */
  /* f3 */ { ".ISB",  INDIRECT_Y, M_INDY, M_INDY, 8 },

  /* f4 */ { ".NOOP", ZERO_PAGE_X, M_NONE, M_NONE, 4 },
  /* f5 */ { "SBC",   ZERO_PAGE_X, M_ZERX, M_AC,   4 }, /* Zeropage,X */
  /* f6 */ { "INC",   ZERO_PAGE_X, M_ZERX, M_ZERX, 6 }, /* Zeropage,X */
  /* f7 */ { ".ISB",  ZERO_PAGE_X, M_ZERX, M_ZERX, 6 },

  /* f8 */ { "SED",   IMPLIED,    M_NONE, M_FD,   2 },
  /* f9 */ { "SBC",   ABSOLUTE_Y, M_ABSY, M_AC,   4 }, /* Absolute,Y */
  /* fa */ { ".NOOP", IMPLIED,    M_NONE, M_NONE, 2 },
  /* fb */ { ".ISB",  ABSOLUTE_Y, M_ABSY, M_ABSY, 7 },

  /* fc */ { ".NOOP", ABSOLUTE_X, M_NONE, M_NONE, 4 },
  /* fd */ { "SBC",   ABSOLUTE_X, M_ABSX, M_AC,   4 }, /* Absolute,X */
  /* fe */ { "INC",   ABSOLUTE_X, M_ABSX, M_ABSX, 7 }, /* Absolute,X */
  /* ff */ { ".ISB",  ABSOLUTE_X, M_ABSX, M_ABSX, 7 }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* DiStella::ourTIAMnemonic[62] = {
  "VSYNC", "VBLANK", "WSYNC", "RSYNC", "NUSIZ0", "NUSIZ1", "COLUP0", "COLUP1",
  "COLUPF", "COLUBK", "CTRLPF", "REFP0", "REFP1", "PF0", "PF1", "PF2", "RESP0",
  "RESP1", "RESM0", "RESM1", "RESBL", "AUDC0", "AUDC1", "AUDF0", "AUDF1",
  "AUDV0", "AUDV1", "GRP0", "GRP1", "ENAM0", "ENAM1", "ENABL", "HMP0", "HMP1",
  "HMM0", "HMM1", "HMBL", "VDELP0", "VDELP1", "VDELBL", "RESMP0", "RESMP1",
  "HMOVE", "HMCLR", "CXCLR", "$2D", "$2E", "$2F", "CXM0P", "CXM1P", "CXP0FB",
  "CXP1FB", "CXM0FB", "CXM1FB", "CXBLPF", "CXPPMM", "INPT0", "INPT1", "INPT2",
  "INPT3", "INPT4", "INPT5"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* DiStella::ourIOMnemonic[24] = {
  "SWCHA", "SWACNT", "SWCHB", "SWBCNT", "INTIM", "$0285", "$0286", "$0287",
  "$0288", "$0289", "$028A", "$028B", "$028C", "$028D", "$028E", "$028F",
  "$0290", "$0291", "$0292", "$0293", "TIM1T", "TIM8T", "TIM64T", "T1024T"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const int DiStella::ourCLength[14] = {
  1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 0
};

#ifdef USE_MAIN
int main(int ac, char* av[])
{
  DiStella dis;
  DisassemblyList list;

  dis.disassemble(list, av[1]);

  printf("Disassembly results:\n\n");
  for(uInt32 i = 0; i < list.size(); ++i)
  {
    const DisassemblyTag& tag = list[i];
    printf("%.4X |   %s %s\n", tag.address, tag.disasm.c_str(), tag.bytes.c_str());
  }

  return 0;
}
#endif
